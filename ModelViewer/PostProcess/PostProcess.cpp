#include "PostProcess.h"
#include "ResourceManager.h"
#include "PostProcessEmpty.h"

ID3D11VertexShader* IPostProcess::ms_QuadVertexShader = nullptr;
Microsoft::WRL::ComPtr<ID3D11InputLayout> IPostProcess::ms_QuadInputLayout;
Microsoft::WRL::ComPtr<ID3D11Buffer> IPostProcess::ms_QuadVertexBuffer;
Microsoft::WRL::ComPtr<ID3D11Buffer> IPostProcess::ms_QuadIndexBuffer;
std::shared_ptr<PostProcessEmpty> IPostProcess::ms_EmptyPostProcess;
std::shared_ptr<Profiler> IPostProcess::ms_Profiler;
const char* IPostProcess::ms_vsFilename = "Shaders/QuadVS.hlsl";
bool IPostProcess::ms_bInitialised = false;

void IPostProcess::RenderControls()
{
	if (m_pOwner == nullptr && ImGui::Button("Reset to defaults"))
	{
		ResetToDefaults();
		for (IPostProcess* pPostProcess : m_OwnedPostProcesses)
		{
			pPostProcess->ResetToDefaults();
		}
	}
}

void IPostProcess::ApplyPostProcess(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV)
{
	Graphics::GetSingletonPtr()->GetDeviceContext()->OMSetRenderTargets(8u, NullRTVs, nullptr);
	Graphics::GetSingletonPtr()->GetDeviceContext()->PSSetShaderResources(0u, 8u, NullSRVs);
	ApplyPostProcessImpl(DeviceContext, RTV, SRV);
}

void IPostProcess::InitStatics(std::shared_ptr<Profiler> Profiler)
{
	HRESULT hResult;
	Microsoft::WRL::ComPtr<ID3D10Blob> vsBuffer;
	D3D11_INPUT_ELEMENT_DESC VertexLayout[3] = {};
	D3D11_DEPTH_STENCIL_DESC DepthStencilDesc = {};
	unsigned int NumElements;

	ms_Profiler = Profiler;
	ms_QuadVertexShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11VertexShader>(ms_vsFilename, "main", vsBuffer);

	VertexLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	VertexLayout[0].SemanticName = "POSITION";
	VertexLayout[0].SemanticIndex = 0;
	VertexLayout[0].InputSlot = 0;
	VertexLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	VertexLayout[0].AlignedByteOffset = 0;
	VertexLayout[0].InstanceDataStepRate = 0;

	VertexLayout[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	VertexLayout[1].SemanticName = "NORMAL";
	VertexLayout[1].SemanticIndex = 0;
	VertexLayout[1].InputSlot = 0;
	VertexLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	VertexLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	VertexLayout[1].InstanceDataStepRate = 0;

	VertexLayout[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	VertexLayout[2].SemanticName = "TEXCOORD";
	VertexLayout[2].SemanticIndex = 0;
	VertexLayout[2].InputSlot = 0;
	VertexLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	VertexLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	VertexLayout[2].InstanceDataStepRate = 0;

	NumElements = sizeof(VertexLayout) / sizeof(VertexLayout[0]);
	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateInputLayout(VertexLayout, NumElements, vsBuffer->GetBufferPointer(),
		vsBuffer->GetBufferSize(), &ms_QuadInputLayout));
	NAME_D3D_RESOURCE(ms_QuadInputLayout, "Post process quad input layout");

	Vertex QuadVertices[] = {
		{ DirectX::XMFLOAT3(-1.0f,  1.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 0.0f), },
		{ DirectX::XMFLOAT3(1.0f,  1.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 0.0f), },
		{ DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 1.0f), },
		{ DirectX::XMFLOAT3(1.0f, -1.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 1.0f), },
	};

	D3D11_BUFFER_DESC BufferDesc = {};
	BufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	BufferDesc.ByteWidth = sizeof(QuadVertices);
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = QuadVertices;

	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &InitData, &ms_QuadVertexBuffer));
	NAME_D3D_RESOURCE(ms_QuadVertexBuffer, "Post process quad vertex buffer");

	unsigned int QuadIndices[] = {
		1, 2, 0,
		3, 2, 1
	};

	BufferDesc = {};
	BufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	BufferDesc.ByteWidth = sizeof(QuadIndices);
	BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	BufferDesc.CPUAccessFlags = 0;

	InitData = {};
	InitData.pSysMem = QuadIndices;

	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &InitData, &ms_QuadIndexBuffer));
	NAME_D3D_RESOURCE(ms_QuadIndexBuffer, "Post process quad index buffer");

	ms_bInitialised = true;
}

void IPostProcess::ShutdownStatics()
{
	ms_QuadInputLayout.Reset();
	ms_QuadVertexBuffer.Reset();
	ms_QuadIndexBuffer.Reset();
	ms_EmptyPostProcess.reset();
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11VertexShader>(ms_vsFilename, "main");

	ms_bInitialised = false;
}

std::shared_ptr<PostProcessEmpty> IPostProcess::GetEmptyPostProcess()
{
	if (ms_EmptyPostProcess.get())
	{
		return IPostProcess::ms_EmptyPostProcess;
	}

	ms_EmptyPostProcess = std::make_shared<PostProcessEmpty>();
	return IPostProcess::ms_EmptyPostProcess;
}

void IPostProcess::ResetToInitialActive()
{
	SetActive(m_bInitialActive);
}

bool IPostProcess::SetupPixelShader(ID3D11PixelShader*& PixelShader, const char* PSFilepath, const char* EntryFunc)
{
	PixelShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11PixelShader>(PSFilepath, EntryFunc);
	assert(PixelShader);

	return true;
}
