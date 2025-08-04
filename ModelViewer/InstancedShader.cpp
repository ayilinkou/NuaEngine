#include "InstancedShader.h"
#include "Light.h"
#include "MyMacros.h"
#include "Common.h"
#include "ResourceManager.h"

InstancedShader::~InstancedShader()
{
	Shutdown();
}

bool InstancedShader::Initialise(ID3D11Device* Device)
{
	bool Result;
	m_vsFilename = "Shaders/InstancedPhongVS.hlsl";
	m_psFilename = "Shaders/PhongPS.hlsl";

	FALSE_IF_FAILED(InitialiseShader(Device));

	return true;
}

void InstancedShader::Shutdown()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11VertexShader>(m_vsFilename, "main");
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, "main");
}

bool InstancedShader::InitialiseShader(ID3D11Device* Device)
{
	HRESULT hResult;
	Microsoft::WRL::ComPtr<ID3D10Blob> vsBuffer;
	D3D11_BUFFER_DESC MatrixBufferDesc = {};
	D3D11_BUFFER_DESC LightBufferDesc = {};
	D3D11_INPUT_ELEMENT_DESC VertexLayout[3] = {};
	unsigned int NumElements;

	m_VertexShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11VertexShader>(m_vsFilename, "main", vsBuffer);
	m_PixelShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11PixelShader>(m_psFilename, "main");

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

	HFALSE_IF_FAILED(Device->CreateInputLayout(VertexLayout, NumElements, vsBuffer->GetBufferPointer(), vsBuffer->GetBufferSize(), &m_InputLayout));
	NAME_D3D_RESOURCE(m_InputLayout, "Instanced shader input layout");

	LightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	LightBufferDesc.ByteWidth = sizeof(LightingBuffer);
	LightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	LightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	HFALSE_IF_FAILED(Device->CreateBuffer(&LightBufferDesc, NULL, &m_LightingBuffer));
	NAME_D3D_RESOURCE(m_LightingBuffer, "Instanced shader lighting buffer");

	return true;
}

bool InstancedShader::SetShaderParameters(ID3D11DeviceContext* DeviceContext, const std::vector<PointLight*>& PointLights,
	const std::vector<DirectionalLight*>& DirLights, const DirectX::XMFLOAT3& SkylightColor)
{
	HRESULT hResult;
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	LightingBuffer* LightingDataPtr;

	ASSERT_NOT_FAILED(DeviceContext->Map(m_LightingBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedResource));
	LightingDataPtr = (LightingBuffer*)MappedResource.pData;
	LightingDataPtr->SkylightColor = SkylightColor;

	int NumDirLights = 0;
	for (int i = 0; i < DirLights.size(); i++)
	{
		assert(NumDirLights < MAX_POINT_LIGHTS);
		LightingDataPtr->DirLights[NumDirLights].LightColor = DirLights[NumDirLights]->GetDiffuseColor();
		LightingDataPtr->DirLights[NumDirLights].LightDir = DirLights[NumDirLights]->GetDirection();
		LightingDataPtr->DirLights[NumDirLights].SpecularPower = DirLights[NumDirLights]->GetSpecularPower();

		NumDirLights++;
		continue;
	}

	LightingDataPtr->DirLightCount = NumDirLights;

	int NumPointLights = 0;
	for (int i = 0; i < PointLights.size(); i++)
	{
		assert(NumPointLights < MAX_POINT_LIGHTS);
		LightingDataPtr->PointLights[NumPointLights].LightColor = PointLights[NumPointLights]->GetDiffuseColor();
		LightingDataPtr->PointLights[NumPointLights].LightPos = PointLights[NumPointLights]->GetPosition();
		LightingDataPtr->PointLights[NumPointLights].Radius = PointLights[NumPointLights]->GetRadius();
		LightingDataPtr->PointLights[NumPointLights].SpecularPower = PointLights[NumPointLights]->GetSpecularPower();

		NumPointLights++;
		continue;
	}

	LightingDataPtr->PointLightCount = NumPointLights;
	DeviceContext->Unmap(m_LightingBuffer.Get(), 0u);

	DeviceContext->PSSetConstantBuffers(1u, 1u, m_LightingBuffer.GetAddressOf());

	return true;
}

void InstancedShader::ActivateShader(ID3D11DeviceContext* DeviceContext)
{
	DeviceContext->IASetInputLayout(m_InputLayout.Get());

	DeviceContext->VSSetShader(m_VertexShader, NULL, 0u);
	DeviceContext->PSSetShader(m_PixelShader, NULL, 0u);
}
