#include "DeferredShader.h"
#include "ResourceManager.h"

DeferredShader::~DeferredShader()
{
	Shutdown();
}

void DeferredShader::Shutdown()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11VertexShader>(m_vsFilename, "main");
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilenameGeo, "main");
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilenameLighting, "main");
}

bool DeferredShader::Initialise(ID3D11Device* Device)
{
	bool Result;
	m_vsFilename = "Shaders/InstancedPhongVS.hlsl";
	m_psFilenameGeo = "Shaders/DeferredGeoPS.hlsl";
	m_psFilenameLighting = "Shaders/LightingPassPS.hlsl";

	FALSE_IF_FAILED(InitialiseShader(Device));

	return true;
}

bool DeferredShader::InitialiseShader(ID3D11Device* Device)
{
	HRESULT hResult;
	Microsoft::WRL::ComPtr<ID3D10Blob> vsBuffer;
	D3D11_INPUT_ELEMENT_DESC VertexLayout[3] = {};
	unsigned int NumElements;

	ms_VertexShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11VertexShader>(m_vsFilename, "main", vsBuffer);
	ms_GeoPassPixelShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11PixelShader>(m_psFilenameGeo);
	ms_LightingPixelShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11PixelShader>(m_psFilenameLighting);

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

	NumElements = _countof(VertexLayout);

	HFALSE_IF_FAILED(Device->CreateInputLayout(VertexLayout, NumElements, vsBuffer->GetBufferPointer(), vsBuffer->GetBufferSize(), &ms_InputLayout));
	NAME_D3D_RESOURCE(ms_InputLayout, "Deferred shader input layout");

	return true;
}

void DeferredShader::ActivateGeoPassShader(ID3D11DeviceContext* pContext)
{
	pContext->IASetInputLayout(ms_InputLayout.Get());

	pContext->VSSetShader(ms_VertexShader, NULL, 0u);
	pContext->PSSetShader(ms_GeoPassPixelShader, NULL, 0u);
}

void DeferredShader::ActivateLightingPassShader(ID3D11DeviceContext* pContext)
{
	pContext->PSSetShader(ms_LightingPixelShader, NULL, 0u);
}