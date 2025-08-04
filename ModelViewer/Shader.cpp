#include "Shader.h"
#include "MyMacros.h"
#include "ResourceManager.h"

Shader::~Shader()
{
	Shutdown();
}

bool Shader::Initialise(ID3D11Device* Device)
{
	bool Result;
	m_vsFilename = "Shaders/PhongVS.hlsl";
	m_psFilename = "Shaders/PhongPS.hlsl";

	FALSE_IF_FAILED(InitialiseShader(Device, m_vsFilename, m_psFilename));

	return true;
}

void Shader::Shutdown()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11VertexShader>(m_vsFilename, "main");
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, "main");
}

bool Shader::InitialiseShader(ID3D11Device* Device, const char* vsFilename, const char* psFilename)
{
	HRESULT hResult;
	Microsoft::WRL::ComPtr<ID3D10Blob> vsBytecode;
	D3D11_BUFFER_DESC MatrixBufferDesc = {};
	D3D11_BUFFER_DESC LightBufferDesc = {};
	D3D11_INPUT_ELEMENT_DESC VertexLayout[3] = {};
	unsigned int NumElements;
	
	m_VertexShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11VertexShader>(vsFilename, "main", vsBytecode);
	m_PixelShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11PixelShader>(psFilename, "main");

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

	HFALSE_IF_FAILED(Device->CreateInputLayout(VertexLayout, NumElements, vsBytecode->GetBufferPointer(), vsBytecode->GetBufferSize(), &m_InputLayout));
	NAME_D3D_RESOURCE(m_InputLayout, "Standard shader input layout");

	MatrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	MatrixBufferDesc.ByteWidth = sizeof(MatrixBuffer);
	MatrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	MatrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	HFALSE_IF_FAILED(Device->CreateBuffer(&MatrixBufferDesc, NULL, &m_MatrixBuffer));
	NAME_D3D_RESOURCE(m_MatrixBuffer, "Standard shader matrix buffer");

	LightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	LightBufferDesc.ByteWidth = sizeof(LightingBuffer);
	LightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	LightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	HFALSE_IF_FAILED(Device->CreateBuffer(&LightBufferDesc, NULL, &m_LightingBuffer));
	NAME_D3D_RESOURCE(m_LightingBuffer, "Standard shader lighting buffer");

	return true;
}

bool Shader::SetShaderParameters(ID3D11DeviceContext* DeviceContext, DirectX::XMMATRIX World, DirectX::XMMATRIX View, DirectX::XMMATRIX Projection,
	DirectX::XMFLOAT3 CameraPos, float Radius, DirectX::XMFLOAT3 LightPos, DirectX::XMFLOAT3 DiffuseColor, float SpecularPower)
{
	HRESULT hResult;
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	MatrixBuffer* MatrixDataPtr;
	LightingBuffer* LightingDataPtr;
	
	// remember to transpose from row major before sending to shaders
	World = DirectX::XMMatrixTranspose(World);
	View = DirectX::XMMatrixTranspose(View);
	Projection = DirectX::XMMatrixTranspose(Projection);
	
	ASSERT_NOT_FAILED(DeviceContext->Map(m_MatrixBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	MatrixDataPtr = (MatrixBuffer*)MappedResource.pData;
	MatrixDataPtr->WorldMatrix = World;
	MatrixDataPtr->ViewMatrix = View;
	MatrixDataPtr->ProjectionMatrix = Projection;
	DeviceContext->Unmap(m_MatrixBuffer.Get(), 0u);

	DeviceContext->VSSetConstantBuffers(1u, 1u, m_MatrixBuffer.GetAddressOf());

	ASSERT_NOT_FAILED(DeviceContext->Map(m_LightingBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedResource));
	LightingDataPtr = (LightingBuffer*)MappedResource.pData;
	LightingDataPtr->CameraPos = CameraPos;
	LightingDataPtr->Radius = Radius;
	LightingDataPtr->LightPos = LightPos;
	LightingDataPtr->SpecularPower = SpecularPower;
	LightingDataPtr->DiffuseColor = DiffuseColor;
	LightingDataPtr->Padding = 0.f;
	DeviceContext->Unmap(m_LightingBuffer.Get(), 0u);

	DeviceContext->PSSetConstantBuffers(1u, 1u, m_LightingBuffer.GetAddressOf());

	return true;
}

void Shader::ActivateShader(ID3D11DeviceContext* DeviceContext)
{
	DeviceContext->IASetInputLayout(m_InputLayout.Get());

	DeviceContext->VSSetShader(m_VertexShader, NULL, 0u);
	DeviceContext->PSSetShader(m_PixelShader, NULL, 0u);
}
