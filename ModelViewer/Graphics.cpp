#include "Graphics.h"

#include "ImGui\imgui_impl_dx11.h"

#include "MyMacros.h"
#include "Light.h"

Graphics* Graphics::m_Instance = nullptr;

Graphics::Graphics()
{
	m_VSync_Enabled = true;
	m_Viewport = {};
	m_VideoCardMemory = -1;
	memset(m_VideoCardDescription, 0, sizeof(m_VideoCardDescription));

	m_FarPlane = 1000.f;
	m_NearPlane = 0.1f;
	m_ScreenAspect = 0.f;

	m_GlobalCBufferData.CameraData.CurrView = DirectX::XMMatrixIdentity();
	m_GlobalCBufferData.CameraData.CurrProj = DirectX::XMMatrixIdentity();
	m_GlobalCBufferData.CameraData.CurrViewProj = DirectX::XMMatrixIdentity();
	m_GlobalCBufferData.CameraData.CurrProjJittered = DirectX::XMMatrixIdentity();
	m_GlobalCBufferData.CameraData.CurrViewProjJittered = DirectX::XMMatrixIdentity();
}

Graphics* Graphics::GetSingletonPtr()
{
	if (!m_Instance)
	{
		m_Instance = new Graphics();
	}
	return m_Instance;
}

bool Graphics::Initialise(int ScreenWidth, int ScreenHeight, bool VSync, HWND hwnd, bool Fullscreen, float ScreenDepth, float ScreenNear)
{
	HRESULT hResult;
	bool Result;
	IDXGIFactory* Factory;
	IDXGIAdapter* Adapter;
	IDXGIOutput* AdapterOutput;
	unsigned int NumModes = 0;
	unsigned int Numerator, Denominator;
	unsigned long long StringLength;
	DXGI_MODE_DESC* DisplayModeList;
	DXGI_ADAPTER_DESC AdapterDesc = {};
	int Error;
	DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
	D3D_FEATURE_LEVEL FeatureLevel;
	ID3D11Texture2D* BackBufferPtr;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> PostProcessRTTFirst;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> PostProcessRTTSecond;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> VelocityBufferTexture;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> NormalBufferTexture;
	D3D11_TEXTURE2D_DESC PostProcessTextureDesc = {};
	D3D11_TEXTURE2D_DESC DepthBufferDesc = {};
	D3D11_TEXTURE2D_DESC NormalBufferDesc = {};
	D3D11_TEXTURE2D_DESC VelocityBufferDesc = {};
	D3D11_DEPTH_STENCIL_DESC DepthStencilDesc = {};
	D3D11_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDesc = {};
	D3D11_SHADER_RESOURCE_VIEW_DESC DepthStencilSRVDesc = {};
	D3D11_RASTERIZER_DESC RasterDesc = {};
	D3D11_SAMPLER_DESC SamplerDesc = {};

	m_VSync_Enabled = VSync;
	m_Dimensions = std::make_pair(ScreenWidth, ScreenHeight);
	m_NearPlane = ScreenNear;
	m_FarPlane = ScreenDepth;
	m_ScreenAspect = (float)ScreenWidth / (float)ScreenHeight;

	ASSERT_NOT_FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&Factory));
	ASSERT_NOT_FAILED(Factory->EnumAdapters(0, &Adapter));
	ASSERT_NOT_FAILED(Adapter->EnumOutputs(0, &AdapterOutput));

	ASSERT_NOT_FAILED(AdapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &NumModes, NULL));

	// create a list to hold all the possible display modes for this monitor/video card combination
	DisplayModeList = new DXGI_MODE_DESC[NumModes];
	if (!DisplayModeList)
	{
		return false;
	}

	ASSERT_NOT_FAILED(AdapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &NumModes, DisplayModeList));

	for (unsigned int i = 0; i < NumModes; i++)
	{
		if (DisplayModeList[i].Width == (unsigned int)ScreenWidth)
		{
			if (DisplayModeList[i].Height == (unsigned int)ScreenHeight)
			{
				Numerator = DisplayModeList[i].RefreshRate.Numerator;
				Denominator = DisplayModeList[i].RefreshRate.Denominator;
			}
		}
	}

	ASSERT_NOT_FAILED(Adapter->GetDesc(&AdapterDesc));

	m_VideoCardMemory = (int)(AdapterDesc.DedicatedVideoMemory / 1024 / 1024);

	Error = wcstombs_s(&StringLength, m_VideoCardDescription, 128, AdapterDesc.Description, 128);
	if (Error != 0)
	{
		return false;
	}

	delete[] DisplayModeList;
	DisplayModeList = 0;

	AdapterOutput->Release();
	AdapterOutput = 0;

	Adapter->Release();
	Adapter = 0;

	Factory->Release();
	Factory = 0;

	SwapChainDesc.BufferCount = 1;
	SwapChainDesc.BufferDesc.Width = ScreenWidth;
	SwapChainDesc.BufferDesc.Height = ScreenHeight;
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // not _SRGB since I'm applying gamma correction manually as a post process
	SwapChainDesc.BufferCount = 1; // create a second buffer and change swapchain swap effect

	if (m_VSync_Enabled)
	{
		SwapChainDesc.BufferDesc.RefreshRate.Numerator = Numerator;
		SwapChainDesc.BufferDesc.RefreshRate.Denominator = Denominator;
	}
	else
	{
		SwapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.OutputWindow = hwnd;

	// turn multisampling off
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;

	if (Fullscreen)
	{
		SwapChainDesc.Windowed = false;
	}
	else
	{
		SwapChainDesc.Windowed = true;
	}

	SwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	SwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // swap to DXGI_SWAP_FLIP_DISCARD when added a second buffer

	FeatureLevel = D3D_FEATURE_LEVEL_11_0;

	ASSERT_NOT_FAILED(D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		D3D11_CREATE_DEVICE_DEBUG,
		&FeatureLevel,
		1,
		D3D11_SDK_VERSION,
		&SwapChainDesc,
		&m_SwapChain,
		&m_Device,
		NULL,
		&m_DeviceContext)
	);
	ASSERT_NOT_FAILED(m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&BackBufferPtr));
	ASSERT_NOT_FAILED(m_Device->CreateRenderTargetView(BackBufferPtr, NULL, &m_BackBufferRTV));
	m_BackBufferRTV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Back buffer RTV"), "Back buffer RTV");

	BackBufferPtr->Release();
	BackBufferPtr = 0;

	DepthBufferDesc.Width = ScreenWidth;
	DepthBufferDesc.Height = ScreenHeight;
	DepthBufferDesc.MipLevels = 1;
	DepthBufferDesc.ArraySize = 1;
	DepthBufferDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	DepthBufferDesc.SampleDesc.Count = 1;
	DepthBufferDesc.SampleDesc.Quality = 0;
	DepthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	ASSERT_NOT_FAILED(m_Device->CreateTexture2D(&DepthBufferDesc, NULL, &m_DepthStencilBuffer));
	m_DepthStencilBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Depth stencil buffer"), "Depth stencil buffer");

	DepthStencilDesc.DepthEnable = true;
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	DepthStencilDesc.StencilEnable = true;
	DepthStencilDesc.StencilReadMask = 0xFF;
	DepthStencilDesc.StencilWriteMask = 0xFF;

	DepthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	DepthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	DepthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	DepthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DepthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	ASSERT_NOT_FAILED(m_Device->CreateDepthStencilState(&DepthStencilDesc, &m_DepthStencilStateWriteEnabled));
	m_DepthStencilStateWriteEnabled->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Depth stencil state write enabled"), "Depth stencil state write enabled");
	
	DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	DepthStencilDesc.StencilWriteMask = 0;

	ASSERT_NOT_FAILED(m_Device->CreateDepthStencilState(&DepthStencilDesc, &m_DepthStencilStateWriteDisabled));
	m_DepthStencilStateWriteDisabled->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Depth stencil state write disabled"), "Depth stencil state write disabled");

	DepthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	DepthStencilDesc.StencilEnable = false;
	DepthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	DepthStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	ASSERT_NOT_FAILED(m_Device->CreateDepthStencilState(&DepthStencilDesc, &m_DepthStencilStateWriteDisabledAlwaysPass));
	m_DepthStencilStateWriteDisabledAlwaysPass->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Depth stencil state write disabled always pass"), "Depth stencil state write disabled always pass");

	m_DeviceContext->OMSetDepthStencilState(m_DepthStencilStateWriteEnabled.Get(), 1);

	DepthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	DepthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

	ASSERT_NOT_FAILED(m_Device->CreateDepthStencilView(m_DepthStencilBuffer.Get(), &DepthStencilViewDesc, &m_DepthStencilView));
	m_DepthStencilView->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Depth stencil view"), "Depth stencil view");

	m_DeviceContext->OMSetRenderTargets(1, m_BackBufferRTV.GetAddressOf(), m_DepthStencilView.Get());

	DepthStencilSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	DepthStencilSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	DepthStencilSRVDesc.Texture2D.MipLevels = 1;

	ASSERT_NOT_FAILED(m_Device->CreateShaderResourceView(m_DepthStencilBuffer.Get(), &DepthStencilSRVDesc, &m_DepthStencilSRV));
	m_DepthStencilSRV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Depth stencil SRV"), "Depth stencil SRV");

	D3D11_BLEND_DESC BlendDesc = {};
	BlendDesc.AlphaToCoverageEnable = false;
	BlendDesc.IndependentBlendEnable = false;
	BlendDesc.RenderTarget[0].BlendEnable = false;
	BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	ASSERT_NOT_FAILED(m_Device->CreateBlendState(&BlendDesc, &m_BlendStateOpaque));
	m_BlendStateOpaque->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Blend state opaque"), "Blend state opaque");
	m_DeviceContext->OMSetBlendState(m_BlendStateOpaque.Get(), nullptr, 0xFFFFFFFF);

	BlendDesc.RenderTarget[0].BlendEnable = true;
	BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	ASSERT_NOT_FAILED(m_Device->CreateBlendState(&BlendDesc, &m_BlendStateTransparent));
	m_BlendStateTransparent->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Blend state transparent"), "Blend state transparent");

	RasterDesc.AntialiasedLineEnable = false;
	RasterDesc.CullMode = D3D11_CULL_BACK;
	RasterDesc.DepthBias = 0;
	RasterDesc.DepthBiasClamp = 0.f;
	RasterDesc.DepthClipEnable = true;
	RasterDesc.FillMode = D3D11_FILL_SOLID;
	RasterDesc.FrontCounterClockwise = false;
	RasterDesc.MultisampleEnable = false;
	RasterDesc.ScissorEnable = false;
	RasterDesc.SlopeScaledDepthBias = 0.f;

	ASSERT_NOT_FAILED(m_Device->CreateRasterizerState(&RasterDesc, &m_RasterStateBackFaceCullOn));
	m_RasterStateBackFaceCullOn->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Raster state back face cull on"), "Raster state back face cull on");

	m_DeviceContext->RSSetState(m_RasterStateBackFaceCullOn.Get());

	RasterDesc.CullMode = D3D11_CULL_NONE;

	ASSERT_NOT_FAILED(m_Device->CreateRasterizerState(&RasterDesc, &m_RasterStateBackFaceCullOff));
	m_RasterStateBackFaceCullOff->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Raster state back face cull off"), "Raster state back face cull off");

	RasterDesc.FillMode = D3D11_FILL_WIREFRAME;

	ASSERT_NOT_FAILED(m_Device->CreateRasterizerState(&RasterDesc, &m_WireframeRasterState));
	NAME_D3D_RESOURCE(m_WireframeRasterState, "Wireframe rasterizer state");

	m_Viewport.Width = (float)ScreenWidth;
	m_Viewport.Height = (float)ScreenHeight;
	m_Viewport.MinDepth = 0.f;
	m_Viewport.MaxDepth = 1.f;
	m_Viewport.TopLeftX = 0.f;
	m_Viewport.TopLeftY = 0.f;

	m_DeviceContext->RSSetViewports(1u, &m_Viewport);

	// setting up render target textures, views and shader resource view for post processing
	PostProcessTextureDesc.Width = ScreenWidth;
	PostProcessTextureDesc.Height = ScreenHeight;
	PostProcessTextureDesc.MipLevels = 1;
	PostProcessTextureDesc.ArraySize = 1;
	PostProcessTextureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	PostProcessTextureDesc.SampleDesc.Count = 1;
	PostProcessTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	PostProcessTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	ASSERT_NOT_FAILED(m_Device->CreateTexture2D(&PostProcessTextureDesc, NULL, &PostProcessRTTFirst));
	ASSERT_NOT_FAILED(m_Device->CreateTexture2D(&PostProcessTextureDesc, NULL, &PostProcessRTTSecond));
	NAME_D3D_RESOURCE(PostProcessRTTFirst, "Post process RTT 1");
	NAME_D3D_RESOURCE(PostProcessRTTSecond, "Post process RTT 2");

	ASSERT_NOT_FAILED(m_Device->CreateRenderTargetView(PostProcessRTTFirst.Get(), NULL, &m_PostProcessRTVFirst));
	ASSERT_NOT_FAILED(m_Device->CreateRenderTargetView(PostProcessRTTSecond.Get(), NULL, &m_PostProcessRTVSecond));
	m_PostProcessRTVFirst->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Post process RTV 1"), "Post process RTV 1");
	m_PostProcessRTVSecond->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Post process RTV 2"), "Post process RTV 2");

	ASSERT_NOT_FAILED(m_Device->CreateShaderResourceView(PostProcessRTTFirst.Get(), NULL, &m_PostProcessSRVFirst));
	ASSERT_NOT_FAILED(m_Device->CreateShaderResourceView(PostProcessRTTSecond.Get(), NULL, &m_PostProcessSRVSecond));
	m_PostProcessSRVFirst->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Post process SRV 1"), "Post process SRV 1");
	m_PostProcessSRVSecond->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Post process SRV 2"), "Post process SRV 2");

	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	ASSERT_NOT_FAILED(m_Device->CreateSamplerState(&SamplerDesc, &m_LinearSampler));
	m_LinearSampler->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Linear sampler state"), "Linear sampler state");

	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

	ASSERT_NOT_FAILED(m_Device->CreateSamplerState(&SamplerDesc, &m_PointSampler));
	m_PointSampler->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen("Point sampler state"), "Point sampler state");

	ID3D11SamplerState* Samplers[2] = { m_LinearSampler.Get(), m_PointSampler.Get() };
	m_DeviceContext->VSSetSamplers(0u, 2u, Samplers);
	m_DeviceContext->HSSetSamplers(0u, 2u, Samplers);
	m_DeviceContext->DSSetSamplers(0u, 2u, Samplers);
	m_DeviceContext->GSSetSamplers(0u, 2u, Samplers);
	m_DeviceContext->PSSetSamplers(0u, 2u, Samplers);
	m_DeviceContext->CSSetSamplers(0u, 2u, Samplers);

	D3D11_QUERY_DESC QueryDesc = {};
	QueryDesc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
	ASSERT_NOT_FAILED(m_Device->CreateQuery(&QueryDesc, &m_PipelineStatsQuery));
	NAME_D3D_RESOURCE(m_PipelineStatsQuery, "Pipeline stats query");

	VelocityBufferDesc.Width = ScreenWidth;
	VelocityBufferDesc.Height = ScreenHeight;
	VelocityBufferDesc.MipLevels = 1;
	VelocityBufferDesc.ArraySize = 1;
	VelocityBufferDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
	VelocityBufferDesc.SampleDesc.Count = 1;
	VelocityBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	VelocityBufferDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	ASSERT_NOT_FAILED(m_Device->CreateTexture2D(&VelocityBufferDesc, NULL, &VelocityBufferTexture));
	ASSERT_NOT_FAILED(m_Device->CreateRenderTargetView(VelocityBufferTexture.Get(), NULL, &m_VelocityRTV));
	ASSERT_NOT_FAILED(m_Device->CreateShaderResourceView(VelocityBufferTexture.Get(), NULL, &m_VelocitySRV));
	NAME_D3D_RESOURCE(VelocityBufferTexture, "Velocity buffer texture");
	NAME_D3D_RESOURCE(m_VelocityRTV, "Velocity buffer RTV");
	NAME_D3D_RESOURCE(m_VelocitySRV, "Velocity buffer SRV");

	Result = CreateGlobalConstantBuffer();
	assert(Result);

	NormalBufferDesc.Width = ScreenWidth;
	NormalBufferDesc.Height = ScreenHeight;
	NormalBufferDesc.MipLevels = 1;
	NormalBufferDesc.ArraySize = 1;
	NormalBufferDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	NormalBufferDesc.SampleDesc.Count = 1;
	NormalBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	NormalBufferDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	ASSERT_NOT_FAILED(m_Device->CreateTexture2D(&NormalBufferDesc, NULL, &NormalBufferTexture));
	ASSERT_NOT_FAILED(m_Device->CreateRenderTargetView(NormalBufferTexture.Get(), NULL, &m_NormalRTV));
	ASSERT_NOT_FAILED(m_Device->CreateShaderResourceView(NormalBufferTexture.Get(), NULL, &m_NormalSRV));
	NAME_D3D_RESOURCE(NormalBufferTexture, "Normal buffer texture");
	NAME_D3D_RESOURCE(m_NormalRTV, "Normal buffer RTV");
	NAME_D3D_RESOURCE(m_NormalSRV, "Normal buffer SRV");

	m_DeviceContext->VSSetConstantBuffers(0u, 1u, m_GlobalCBuffer.GetAddressOf());
	m_DeviceContext->HSSetConstantBuffers(0u, 1u, m_GlobalCBuffer.GetAddressOf());
	m_DeviceContext->DSSetConstantBuffers(0u, 1u, m_GlobalCBuffer.GetAddressOf());
	m_DeviceContext->GSSetConstantBuffers(0u, 1u, m_GlobalCBuffer.GetAddressOf());
	m_DeviceContext->PSSetConstantBuffers(0u, 1u, m_GlobalCBuffer.GetAddressOf());
	m_DeviceContext->CSSetConstantBuffers(0u, 1u, m_GlobalCBuffer.GetAddressOf());

	ImGui_ImplDX11_Init(m_Device.Get(), m_DeviceContext.Get());

	return true;
}

void Graphics::Shutdown()
{
	if (m_SwapChain)
	{
		m_SwapChain->SetFullscreenState(false, NULL);
	}

	m_DeviceContext->ClearState();
	m_DeviceContext->Flush();

	m_DeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	m_DeviceContext->RSSetState(nullptr);

	m_DepthStencilBuffer.Reset();
	m_DepthStencilStateWriteEnabled.Reset();
	m_DepthStencilStateWriteDisabled.Reset();
	m_DepthStencilStateWriteDisabledAlwaysPass.Reset();
	m_DepthStencilView.Reset();
	m_BlendStateOpaque.Reset();
	m_BlendStateTransparent.Reset();
	m_RasterStateBackFaceCullOn.Reset();
	m_RasterStateBackFaceCullOff.Reset();
	m_LinearSampler.Reset();
	m_PointSampler.Reset();
	m_BackBufferRTV.Reset();
	m_PostProcessRTVFirst.Reset();
	m_PostProcessRTVSecond.Reset();
	m_PostProcessSRVFirst.Reset();
	m_PostProcessSRVSecond.Reset();

	ImGui_ImplDX11_Shutdown();

	m_SwapChain.Reset();

	m_DeviceContext->ClearState();
	m_DeviceContext->Flush();

	Microsoft::WRL::ComPtr<ID3D11Debug> d3dDebug;
	if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&d3dDebug))))
	{
		d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	}

	m_DeviceContext.Reset();
	m_Device.Reset();
}

void Graphics::BeginScene(float Red, float Green, float Blue, float Alpha)
{
	float Color[4];
	Color[0] = Red;
	Color[1] = Green;
	Color[2] = Blue;
	Color[3] = Alpha;

	m_DeviceContext->ClearRenderTargetView(m_NormalRTV.Get(), Color);
	m_DeviceContext->ClearRenderTargetView(m_VelocityRTV.Get(), Color);
	m_DeviceContext->ClearRenderTargetView(m_BackBufferRTV.Get(), Color);
	m_DeviceContext->ClearRenderTargetView(m_PostProcessRTVFirst.Get(), Color);
	m_DeviceContext->ClearRenderTargetView(m_PostProcessRTVSecond.Get(), Color);
	m_DeviceContext->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.f, 0u);
}

void Graphics::EndScene()
{
	if (m_VSync_Enabled)
	{
		m_SwapChain->Present(1u, 0u);
	}
	else
	{
		m_SwapChain->Present(0u, 0u);
	}
}

void Graphics::GetVideoCardInfo(char* CardName, int& Memory)
{
	strcpy_s(CardName, 128, m_VideoCardDescription);
	Memory = m_VideoCardMemory;
}

void Graphics::SetBackBufferRenderTarget()
{
	m_DeviceContext->OMSetRenderTargets(1u, m_BackBufferRTV.GetAddressOf(), m_DepthStencilView.Get());
}

void Graphics::EnableDepthWrite()
{
	m_DeviceContext->OMSetDepthStencilState(m_DepthStencilStateWriteEnabled.Get(), 1);
}

void Graphics::DisableDepthWrite()
{
	m_DeviceContext->OMSetDepthStencilState(m_DepthStencilStateWriteDisabled.Get(), 1);
}

void Graphics::DisableDepthWriteAlwaysPass()
{
	m_DeviceContext->OMSetDepthStencilState(m_DepthStencilStateWriteDisabledAlwaysPass.Get(), 1);
}

void Graphics::EnableBlending()
{
	m_DeviceContext->OMSetBlendState(m_BlendStateTransparent.Get(), nullptr, 0xFFFFFFFF);
}

void Graphics::DisableBlending()
{
	m_DeviceContext->OMSetBlendState(m_BlendStateOpaque.Get(), nullptr, 0xFFFFFFFF);
}

void Graphics::ResetViewport()
{
	m_DeviceContext->RSSetViewports(1u, &m_Viewport);
}

void Graphics::SetRasterStateBackFaceCull(bool bShouldCull)
{
	GetDeviceContext()->RSSetState(bShouldCull ? m_RasterStateBackFaceCullOn.Get() : m_RasterStateBackFaceCullOff.Get());
}

void Graphics::SetWireframeRasterState()
{
	GetDeviceContext()->RSSetState(m_WireframeRasterState.Get());
}

bool Graphics::CreateGlobalConstantBuffer()
{
	D3D11_BUFFER_DESC Desc = {};
	Desc.ByteWidth = sizeof(GlobalCBuffer);
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	
	return SUCCEEDED(m_Device->CreateBuffer(&Desc, nullptr, &m_GlobalCBuffer));
}

void Graphics::UpdateGlobalConstantBuffer(const GlobalCBuffer& NewGlobalCBufferData)
{
	// previous matrices were already transposed and so we can just copy
	m_GlobalCBufferData.CameraData.PrevView = m_GlobalCBufferData.CameraData.CurrView;
	m_GlobalCBufferData.CameraData.PrevProj = m_GlobalCBufferData.CameraData.CurrProj;
	m_GlobalCBufferData.CameraData.PrevViewProj = m_GlobalCBufferData.CameraData.CurrViewProj;
	m_GlobalCBufferData.CameraData.PrevProjJittered = m_GlobalCBufferData.CameraData.CurrProjJittered;
	m_GlobalCBufferData.CameraData.PrevViewProjJittered = m_GlobalCBufferData.CameraData.CurrViewProjJittered;
	m_GlobalCBufferData.PrevTime = m_GlobalCBufferData.CurrTime;

	m_GlobalCBufferData.CameraData.CurrView = DirectX::XMMatrixTranspose(NewGlobalCBufferData.CameraData.CurrView);
	m_GlobalCBufferData.CameraData.CurrProj = DirectX::XMMatrixTranspose(NewGlobalCBufferData.CameraData.CurrProj);
	m_GlobalCBufferData.CameraData.CurrViewProj = DirectX::XMMatrixTranspose(NewGlobalCBufferData.CameraData.CurrViewProj);
	m_GlobalCBufferData.CameraData.CurrProjJittered = DirectX::XMMatrixTranspose(NewGlobalCBufferData.CameraData.CurrProjJittered);
	m_GlobalCBufferData.CameraData.CurrViewProjJittered = DirectX::XMMatrixTranspose(NewGlobalCBufferData.CameraData.CurrViewProjJittered);
	m_GlobalCBufferData.CameraData.InverseProj = NewGlobalCBufferData.CameraData.InverseProj;
	m_GlobalCBufferData.CameraData.ActiveCameraPos = NewGlobalCBufferData.CameraData.ActiveCameraPos;
	m_GlobalCBufferData.CameraData.MainCameraPos = NewGlobalCBufferData.CameraData.MainCameraPos;
	m_GlobalCBufferData.CurrTime = NewGlobalCBufferData.CurrTime;
	m_GlobalCBufferData.NearZ = NewGlobalCBufferData.NearZ;
	m_GlobalCBufferData.FarZ = NewGlobalCBufferData.FarZ;
	m_GlobalCBufferData.ScreenRes = DirectX::XMFLOAT2((float)m_Dimensions.first, (float)m_Dimensions.second);
	m_GlobalCBufferData.LightData.SkylightColor = NewGlobalCBufferData.LightData.SkylightColor;
	
	memcpy(m_GlobalCBufferData.CameraData.FrustumPlanes, NewGlobalCBufferData.CameraData.FrustumPlanes, sizeof(DirectX::XMFLOAT4) * 6);

	std::vector<PointLight*> PointLights;
	std::vector<DirectionalLight*> DirLights;
	for (Light* pLight : Light::GetLights())
	{
		if (pLight && pLight->IsActive())
		{
			PointLight* pPointLight = dynamic_cast<PointLight*>(pLight);
			if (pPointLight)
			{
				PointLights.push_back(pPointLight);
				continue;
			}

			DirectionalLight* pDirLight = dynamic_cast<DirectionalLight*>(pLight);
			if (pDirLight)
			{
				DirLights.push_back(pDirLight);
				continue;
			}
		}
	}

	int NumDirLights = 0;
	for (int i = 0; i < DirLights.size(); i++)
	{
		assert(NumDirLights < MAX_POINT_LIGHTS);
		m_GlobalCBufferData.LightData.DirLights[NumDirLights].LightColor = DirLights[NumDirLights]->GetDiffuseColor();
		m_GlobalCBufferData.LightData.DirLights[NumDirLights].LightDir = DirLights[NumDirLights]->GetDirection();
		m_GlobalCBufferData.LightData.DirLights[NumDirLights].SpecularPower = DirLights[NumDirLights]->GetSpecularPower();

		NumDirLights++;
		continue;
	}
	m_GlobalCBufferData.LightData.DirLightCount = NumDirLights;

	int NumPointLights = 0;
	for (int i = 0; i < PointLights.size(); i++)
	{
		assert(NumPointLights < MAX_POINT_LIGHTS);
		m_GlobalCBufferData.LightData.PointLights[NumPointLights].LightColor = PointLights[NumPointLights]->GetDiffuseColor();
		m_GlobalCBufferData.LightData.PointLights[NumPointLights].LightPos = PointLights[NumPointLights]->GetPosition();
		m_GlobalCBufferData.LightData.PointLights[NumPointLights].Radius = PointLights[NumPointLights]->GetRadius();
		m_GlobalCBufferData.LightData.PointLights[NumPointLights].SpecularPower = PointLights[NumPointLights]->GetSpecularPower();

		NumPointLights++;
		continue;
	}
	m_GlobalCBufferData.LightData.PointLightCount = NumPointLights;

	HRESULT hResult;
	D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
	ASSERT_NOT_FAILED(m_DeviceContext->Map(m_GlobalCBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedSubresource));
	memcpy(MappedSubresource.pData, &m_GlobalCBufferData, sizeof(GlobalCBuffer));
	m_DeviceContext->Unmap(m_GlobalCBuffer.Get(), 0u);
}

DirectX::XMMATRIX Graphics::GetDefaultProjMatrix() const
{
	float FieldOfView = 3.141592654f / 4.f;
	return DirectX::XMMatrixPerspectiveFovLH(FieldOfView, m_ScreenAspect, m_NearPlane, m_FarPlane);
}
