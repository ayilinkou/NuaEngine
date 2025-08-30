#include "PostProcessBloom.h"
#include "PostProcessGaussianBlur.h"
#include "ResourceManager.h"

PostProcessBloom::PostProcessBloom(bool bActive, const PostProcessData::BloomData& Data, const PostProcessData::GaussianBlurData& GaussianData,
	const std::pair<int, int>& Dimensions) : PostProcess(bActive, Data, "Bloom", false), m_psFilename("Shaders/BloomPS.hlsl")
{
	m_LuminanceEntry = "LuminancePS";
	m_BloomEntry = "BloomPS";

	HRESULT hResult;
	D3D11_BUFFER_DESC BufferDesc = {};
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4);
	BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	DirectX::XMFLOAT4 LuminanceData = DirectX::XMFLOAT4(Data.LuminanceThreshold, 0.f, 0.f, 0.f);
	D3D11_SUBRESOURCE_DATA BufferData = {};
	BufferData.pSysMem = &LuminanceData;

	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &BufferData, &m_ConstantBuffer));
	NAME_D3D_RESOURCE(m_ConstantBuffer, ("Post process " + m_Name + " constant buffer").c_str());

	SetupPixelShader(m_LuminancePS, m_psFilename, m_LuminanceEntry);
	SetupPixelShader(m_BloomPS, m_psFilename, m_BloomEntry);

	D3D11_TEXTURE2D_DESC TextureDesc = {};
	TextureDesc.Width = Dimensions.first;
	TextureDesc.Height = Dimensions.second;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	ID3D11Texture2D* LuminousTexture;
	ID3D11Texture2D* BlurredTexture;
	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateTexture2D(&TextureDesc, nullptr, &LuminousTexture));
	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateTexture2D(&TextureDesc, nullptr, &BlurredTexture));
	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateRenderTargetView(LuminousTexture, NULL, &m_LuminousRTV));
	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateRenderTargetView(BlurredTexture, NULL, &m_BlurredRTV));
	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateShaderResourceView(LuminousTexture, NULL, &m_LuminousSRV));
	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateShaderResourceView(BlurredTexture, NULL, &m_BlurredSRV));

	NAME_D3D_RESOURCE(LuminousTexture, ("Post process " + m_Name + " luminous texture").c_str());
	NAME_D3D_RESOURCE(BlurredTexture, ("Post process " + m_Name + " blurred texture").c_str());
	NAME_D3D_RESOURCE(m_LuminousRTV, ("Post process " + m_Name + " luminous texture RTV").c_str());
	NAME_D3D_RESOURCE(m_BlurredRTV, ("Post process " + m_Name + " blurred texture RTV").c_str());
	NAME_D3D_RESOURCE(m_LuminousSRV, ("Post process " + m_Name + " luminous texture SRV").c_str());
	NAME_D3D_RESOURCE(m_BlurredSRV, ("Post process " + m_Name + " blurred texture SRV").c_str());

	LuminousTexture->Release();
	BlurredTexture->Release();

	m_BlurPostProcess = std::make_unique<PostProcessGaussianBlur>(true, GaussianData, Dimensions);
	m_BlurPostProcess->SetOwner(this);
	m_OwnedPostProcesses.push_back(m_BlurPostProcess.get());

	m_OnResetToDefaults.Bind(std::function<void()>([this]() { this->m_BlurPostProcess->ResetToDefaults(); }));
}

PostProcessBloom::~PostProcessBloom()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, m_LuminanceEntry);
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, m_BloomEntry);
}

void PostProcessBloom::RenderControls()
{
	bool bDirty = false;

	if (ImGui::SliderFloat("Luminance Threshold", &m_CurrentSettings.LuminanceThreshold, 0.f, 5.f, "%.2f"))
		bDirty = true;

	if (bDirty)
		UpdateConstantBuffer();

	m_BlurPostProcess->RenderControls();
	IPostProcess::RenderControls();
}

void PostProcessBloom::ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV)
{
	// render luminous pixels
	DeviceContext->PSSetShader(m_LuminancePS, nullptr, 0u);
	DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());
	DeviceContext->PSSetConstantBuffers(1u, 1u, m_ConstantBuffer.GetAddressOf());

	DeviceContext->OMSetRenderTargets(1u, m_LuminousRTV.GetAddressOf(), nullptr);
	DeviceContext->DrawIndexed(6u, 0u, 0);
	GetProfiler()->AddDrawCall();

	// blur luminous pixels
	m_BlurPostProcess->ApplyPostProcess(DeviceContext, m_BlurredRTV, m_LuminousSRV);

	// add bloom to original
	ID3D11RenderTargetView* NullRTVs[] = { nullptr };
	Graphics::GetSingletonPtr()->GetDeviceContext()->OMSetRenderTargets(1u, NullRTVs, nullptr);

	DeviceContext->PSSetShader(m_BloomPS, nullptr, 0u);
	DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());
	DeviceContext->PSSetShaderResources(1u, 1u, m_BlurredSRV.GetAddressOf());
	DeviceContext->PSSetConstantBuffers(1u, 1u, m_ConstantBuffer.GetAddressOf());

	DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);
	DeviceContext->DrawIndexed(6u, 0u, 0);
	GetProfiler()->AddDrawCall();
}