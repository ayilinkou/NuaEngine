#include <algorithm>

#include "d3dcompiler.h"

#include "BoxRenderer.h"
#include "Graphics.h"
#include "MyMacros.h"
#include "Camera.h"
#include "Application.h"
#include "AABB.h"
#include "ResourceManager.h"
#include "CameraManager.h"

const UINT BoxIndices[12][2] = {
	{0, 1}, {1, 3}, {3, 2}, {2, 0},
	{4, 5}, {5, 7}, {7, 6}, {6, 4},
	{0, 4}, {1, 5}, {2, 6}, {3, 7}
};

BoxRenderer::BoxRenderer() : m_PixelShader(nullptr), m_VertexShader(nullptr)
{
}

BoxRenderer::~BoxRenderer()
{
	Shutdown();
}

bool BoxRenderer::Init(std::shared_ptr<CameraManager> CamManager)
{
	m_CameraManager = CamManager;
	
	bool Result;
	FALSE_IF_FAILED(CreateShaders());
	FALSE_IF_FAILED(CreateBuffers());
	FALSE_IF_FAILED(CreateViews());

	return true;
}

void BoxRenderer::Shutdown()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11VertexShader>(m_vsFilename, "main");
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, "main");
}

void BoxRenderer::ClearBoxes()
{
	m_Boxes.clear();
}

void BoxRenderer::Render()
{
	if (m_Boxes.size() == 0)
		return;
	
	Graphics::GetSingletonPtr()->DisableBlending();
	Graphics::GetSingletonPtr()->EnableDepthWrite();

	UpdateBuffers();

	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();
	UINT Strides[] = { sizeof(DirectX::XMFLOAT4) };
	UINT Offsets[] = { 0u };
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	DeviceContext->IASetInputLayout(m_InputLayout.Get());
	DeviceContext->IASetVertexBuffers(0u, 1u, m_VertexBuffer.GetAddressOf(), Strides, Offsets);
	DeviceContext->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0u);

	DeviceContext->VSSetShader(m_VertexShader, nullptr, 0u);
	DeviceContext->VSSetConstantBuffers(0u, 1u, m_CameraCBuffer.GetAddressOf());
	DeviceContext->VSSetShaderResources(0u, 1u, m_CornersSRV.GetAddressOf());

	DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);

	Graphics::GetSingletonPtr()->DisableDepthWrite();
	Graphics::GetSingletonPtr()->DisableBlending();

	UINT InstancesLeft = (UINT)m_Boxes.size();
	UINT InstanceOffset = 0u;
	UINT DrawCallsNeeded = (InstancesLeft + MAX_INSTANCE_COUNT - 1) / MAX_INSTANCE_COUNT;

	for (UINT i = 0; i < DrawCallsNeeded; i++)
	{
#undef min
		UINT InstanceCount = std::min(InstancesLeft, (UINT)MAX_INSTANCE_COUNT);

		UpdateCornersBuffer(InstanceOffset);
		DeviceContext->DrawIndexedInstanced(24u, InstanceCount, 0u, 0u, 0u);
		Application::GetSingletonPtr()->GetRenderStatsRef().DrawCalls++;

		InstanceOffset += InstanceCount;
		InstancesLeft -= InstanceCount;
	}
}

bool BoxRenderer::CreateShaders()
{
	HRESULT hResult;
	Microsoft::WRL::ComPtr<ID3D10Blob> vsBuffer;

	m_vsFilename = "Shaders/BoxRendererVS.hlsl";
	m_psFilename = "Shaders/BoxRendererPS.hlsl";

	m_VertexShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11VertexShader>(m_vsFilename, "main", vsBuffer);
	m_PixelShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11PixelShader>(m_psFilename, "main");

	D3D11_INPUT_ELEMENT_DESC LayoutDesc[1] = {};
	LayoutDesc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	LayoutDesc[0].SemanticName = "POSITION";
	LayoutDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateInputLayout(LayoutDesc, _countof(LayoutDesc), vsBuffer->GetBufferPointer(), vsBuffer->GetBufferSize(), &m_InputLayout));
	NAME_D3D_RESOURCE(m_InputLayout, "Box renderer input layout");

	return true;
}

bool BoxRenderer::CreateBuffers()
{
	HRESULT hResult;

	D3D11_BUFFER_DESC Desc = {};
	Desc.Usage = D3D11_USAGE_IMMUTABLE;
	Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	Desc.ByteWidth = sizeof(BoxIndices);

	D3D11_SUBRESOURCE_DATA Data = {};
	Data.pSysMem = BoxIndices;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, &Data, &m_IndexBuffer));
	NAME_D3D_RESOURCE(m_IndexBuffer, "Box renderer index buffer");

	Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	Desc.ByteWidth = sizeof(DirectX::XMFLOAT4) * 8;

	AABB BBox = {};
	BBox.Min = { -0.5f, -0.5f, -0.5f };
	BBox.Max = {  0.5f,  0.5f,  0.5f };
	BBox.CalcCorners();

	Data = {};
	Data.pSysMem = BBox.Corners.data();

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, &Data, &m_VertexBuffer));
	NAME_D3D_RESOURCE(m_VertexBuffer, "Box renderer vertex buffer");

	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	Desc.ByteWidth = sizeof(CameraBuffer);

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, nullptr, &m_CameraCBuffer));
	NAME_D3D_RESOURCE(m_CameraCBuffer, "Box renderer camera constant buffer");

	Desc.ByteWidth = sizeof(CornersBuffer);
	Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	Desc.StructureByteStride = sizeof(DirectX::XMFLOAT4);

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, nullptr, &m_CornersBuffer));
	NAME_D3D_RESOURCE(m_CornersBuffer, "Box renderer corners structured buffer");

	return true;
}

bool BoxRenderer::CreateViews()
{
	HRESULT hResult;
	D3D11_SHADER_RESOURCE_VIEW_DESC Desc = {};
	Desc.Format = DXGI_FORMAT_UNKNOWN;
	Desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	Desc.Buffer.NumElements = MAX_INSTANCE_COUNT * 8;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateShaderResourceView(m_CornersBuffer.Get(), &Desc, &m_CornersSRV));
	NAME_D3D_RESOURCE(m_CornersSRV, "Box renderer corners SRV");

	return true;
}

void BoxRenderer::UpdateBuffers()
{
	HRESULT hResult;
	CameraBuffer* CameraBufferPtr;
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();

	ASSERT_NOT_FAILED(DeviceContext->Map(m_CameraCBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	CameraBufferPtr = (CameraBuffer*)MappedResource.pData;
	CameraBufferPtr->ViewProj = DirectX::XMMatrixTranspose(m_CameraManager->GetActiveCamera()->GetViewProjMatrix());
	DeviceContext->Unmap(m_CameraCBuffer.Get(), 0u);
}

void BoxRenderer::UpdateCornersBuffer(const UINT StartInstance)
{
	HRESULT hResult;
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();
	
	UINT InstancesLeft = (UINT)m_Boxes.size() - StartInstance;
	UINT InstanceCount = std::min(InstancesLeft, (UINT)MAX_INSTANCE_COUNT);

	ASSERT_NOT_FAILED(DeviceContext->Map(m_CornersBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	memcpy(MappedResource.pData, m_Boxes.data() + StartInstance, sizeof(DirectX::XMFLOAT4) * 8 * InstanceCount);
	DeviceContext->Unmap(m_CornersBuffer.Get(), 0u);
}

void BoxRenderer::LoadBoxCorners(const AABB& BBox, const DirectX::XMMATRIX& Transform)
{
	std::array<DirectX::XMFLOAT4, 8> Corners;

	for (size_t i = 0; i < 8; i++)
	{
		DirectX::XMVECTOR Corner = DirectX::XMVectorSet(BBox.Corners[i].x, BBox.Corners[i].y, BBox.Corners[i].z, 1.f);
		DirectX::XMVECTOR WorldCorner = DirectX::XMVector4Transform(Corner, Transform);

		DirectX::XMStoreFloat4(&Corners[i], WorldCorner);
	}

	m_Boxes.push_back(Corners);
}

void BoxRenderer::LoadFrustumCorners(const std::shared_ptr<Camera>& pCamera)
{
	std::array<DirectX::XMFLOAT4, 8> Corners;
	DirectX::XMMATRIX ViewProj = DirectX::XMMatrixMultiply(pCamera->GetViewMatrix(), pCamera->GetProjMatrix());
	DirectX::XMMATRIX InvViewProj = DirectX::XMMatrixInverse(nullptr, ViewProj);

	int i = 0;
	for (int z = 0; z <= 1; ++z)
	{
		float ndcZ = z * 2.0f - 1.0f;
		for (int y = 0; y <= 1; ++y)
		{
			float ndcY = y * 2.0f - 1.0f;
			for (int x = 0; x <= 1; ++x)
			{
				float ndcX = x * 2.0f - 1.0f;

				DirectX::XMVECTOR Corner = DirectX::XMVectorSet(ndcX, ndcY, ndcZ, 1.0f);
				DirectX::XMVECTOR WorldCorner = DirectX::XMVector4Transform(Corner, InvViewProj);
				WorldCorner = DirectX::XMVectorScale(WorldCorner, 1.0f / DirectX::XMVectorGetW(WorldCorner));

				DirectX::XMStoreFloat4(&Corners[i++], WorldCorner);
			}
		}
	}

	m_Boxes.push_back(Corners);
}
