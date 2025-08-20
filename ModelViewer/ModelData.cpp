#include "ModelData.h"

#include <fstream>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "MyMacros.h"
#include "Graphics.h"
#include "ResourceManager.h"
#include "InstancedShader.h"
#include "Node.h"
#include "Material.h"
#include "Common.h"
#include "FrustumCuller.h"
#include "Profiler.h"

ModelData::ModelData(const std::string& ModelPath, FrustumCuller* pFrustumCuller, std::shared_ptr<Profiler> pProfiler,
	const std::string& TexturesPath) : m_FrustumCuller(pFrustumCuller), m_Profiler(pProfiler)
{
	assert(Initialise(Graphics::GetSingletonPtr()->GetDevice(), Graphics::GetSingletonPtr()->GetDeviceContext(), ModelPath, TexturesPath));
}

ModelData::~ModelData()
{
	Shutdown();
}

bool ModelData::Initialise(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext, const std::string& ModelFilename, const std::string& TexturesPath)
{
	bool Result;
	m_ModelPath = ModelFilename;
	m_TexturesPath = TexturesPath;

	FALSE_IF_FAILED(LoadModel());

	return true;
}

void ModelData::Shutdown()
{
	Reset();
}

void ModelData::Render()
{
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();
	UINT Strides[] = { sizeof(Vertex) };
	UINT Offsets[] = { 0u, };

	DeviceContext->IASetVertexBuffers(0u, 1u, m_VertexBuffer.GetAddressOf(), Strides, Offsets);
	DeviceContext->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0u);
	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	DeviceContext->VSSetShaderResources(0u, 1u, m_CulledTransformsSRV.GetAddressOf());

	Graphics::GetSingletonPtr()->EnableDepthWrite();
	Graphics::GetSingletonPtr()->DisableBlending();
	InstancedShader::ActivateShaderOpaque(DeviceContext);
	RenderMeshes(m_OpaqueMeshes);

	if (!m_TransparentMeshes.empty())
	{
		Graphics::GetSingletonPtr()->DisableDepthWrite();
		Graphics::GetSingletonPtr()->EnableBlending();
		InstancedShader::ActivateShaderTransparent(DeviceContext);
		RenderMeshes(m_TransparentMeshes);
	}

	ID3D11ShaderResourceView* NullSRVs[] = { nullptr };
	DeviceContext->VSSetShaderResources(0u, 1u, NullSRVs);
}

void ModelData::ShutdownBuffers()
{
	m_VertexBuffer.Reset();
	m_IndexBuffer.Reset();
}

bool ModelData::LoadModel()
{
	Reset();

	Assimp::Importer Importer;
	const aiScene* Scene = Importer.ReadFile(m_ModelPath,
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenSmoothNormals |
		aiProcess_ConvertToLeftHanded
	);

	assert(Scene);

	LoadMaterials(Scene);
	m_RootNode = std::make_unique<Node>(this, nullptr);
	m_RootNode->ProcessNode(Scene->mRootNode, Scene, DirectX::XMMatrixIdentity());
	m_BoundingBox.CalcCorners();

	bool Result;
	Result = CreateBuffers();
	assert(Result);
	Result = CreateViews();
	assert(Result);

	return true;
}

void ModelData::ReleaseModel()
{
	m_RootNode.reset();

	m_Transforms.clear();
	m_Textures.clear();
	m_Materials.clear();
	m_Meshes.clear();
	m_OpaqueMeshes.clear();
	m_TransparentMeshes.clear();
	m_Vertices.clear();
	m_Indices.clear();
	m_BoundingBox = {};

	for (const std::string& Path : m_TexturePathsSet)
	{
		ResourceManager::GetSingletonPtr()->UnloadTexture(Path);
	}
	m_TexturePathsSet.clear();

	m_Textures.shrink_to_fit();
	m_Materials.shrink_to_fit();
	m_OpaqueMeshes.shrink_to_fit();
	m_TransparentMeshes.shrink_to_fit();
	m_Vertices.shrink_to_fit();
	m_Indices.shrink_to_fit();
}

void ModelData::Reset()
{
	ShutdownBuffers();
	m_TextureIndexMap.clear();
	ReleaseModel();
}

bool ModelData::CreateBuffers()
{
	HRESULT hResult;
	D3D11_BUFFER_DESC Desc = {};
	D3D11_SUBRESOURCE_DATA Data = {};
	ID3D11Device* Device = Graphics::GetSingletonPtr()->GetDevice();

	Desc.Usage = D3D11_USAGE_IMMUTABLE;
	Desc.ByteWidth = (UINT)(sizeof(Vertex) * m_Vertices.size());
	Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	Data.pSysMem = m_Vertices.data();

	HFALSE_IF_FAILED(Device->CreateBuffer(&Desc, &Data, &m_VertexBuffer));
	NAME_D3D_RESOURCE(m_VertexBuffer, (m_ModelPath + " vertex buffer").c_str());

	Desc.ByteWidth = (UINT)(sizeof(unsigned int) * m_Indices.size());
	Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	Data.pSysMem = m_Indices.data();

	HFALSE_IF_FAILED(Device->CreateBuffer(&Desc, &Data, &m_IndexBuffer));
	NAME_D3D_RESOURCE(m_IndexBuffer, (m_ModelPath + " index buffer").c_str());

	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.ByteWidth = (UINT)(sizeof(CullTransformData) * MAX_INSTANCE_COUNT);
	Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	Desc.StructureByteStride = sizeof(CullTransformData);

	HFALSE_IF_FAILED(Device->CreateBuffer(&Desc, nullptr, &m_TransformsBuffer));
	NAME_D3D_RESOURCE(m_TransformsBuffer, (m_ModelPath + " transforms buffer").c_str());

	Desc.CPUAccessFlags = 0;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

	HFALSE_IF_FAILED(Device->CreateBuffer(&Desc, nullptr, &m_CulledTransformsBuffer));
	NAME_D3D_RESOURCE(m_CulledTransformsBuffer, (m_ModelPath + " culled transforms buffer").c_str());

	Desc.ByteWidth = sizeof(UINT) * 2;
	Desc.StructureByteStride = sizeof(UINT);
	Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	HFALSE_IF_FAILED(Device->CreateBuffer(&Desc, nullptr, &m_InstanceCountBuffer));
	NAME_D3D_RESOURCE(m_InstanceCountBuffer, (m_ModelPath + " instance count buffer").c_str());

	return true;
}

bool ModelData::CreateViews()
{
	HRESULT hResult;
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	ID3D11Device* Device = Graphics::GetSingletonPtr()->GetDevice();

	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.NumElements = (UINT)MAX_INSTANCE_COUNT;
	uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;

	HFALSE_IF_FAILED(Device->CreateUnorderedAccessView(m_CulledTransformsBuffer.Get(), &uavDesc, &m_CulledTransformsUAV));
	NAME_D3D_RESOURCE(m_CulledTransformsUAV, (m_ModelPath + " culled transforms buffer UAV").c_str());

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.Buffer.NumElements = (UINT)MAX_INSTANCE_COUNT;

	HFALSE_IF_FAILED(Device->CreateShaderResourceView(m_TransformsBuffer.Get(), &SRVDesc, &m_TransformsSRV));
	NAME_D3D_RESOURCE(m_TransformsSRV, (m_ModelPath + " transforms buffer SRV").c_str());

	HFALSE_IF_FAILED(Device->CreateShaderResourceView(m_CulledTransformsBuffer.Get(), &SRVDesc, &m_CulledTransformsSRV));
	NAME_D3D_RESOURCE(m_CulledTransformsSRV, (m_ModelPath + " culled transforms buffer SRV").c_str());

	uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.Buffer.NumElements = 2;

	HFALSE_IF_FAILED(Device->CreateUnorderedAccessView(m_InstanceCountBuffer.Get(), &uavDesc, &m_InstanceCountUAV));
	NAME_D3D_RESOURCE(m_InstanceCountUAV, (m_ModelPath + " instance count buffer UAV").c_str());

	return true;
}

void ModelData::UpdateBuffers()
{
	assert(m_Transforms.size() <= MAX_INSTANCE_COUNT);

	HRESULT hResult;
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();
	D3D11_MAPPED_SUBRESOURCE MappedResource = {};

	ASSERT_NOT_FAILED(DeviceContext->Map(m_TransformsBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedResource));
	memcpy(MappedResource.pData, m_Transforms.data(), sizeof(CullTransformData) * m_Transforms.size());
	DeviceContext->Unmap(m_TransformsBuffer.Get(), 0u);
}

void ModelData::LoadMaterials(const aiScene* Scene)
{
	for (size_t i = 0; i < Scene->mNumMaterials; i++)
	{
		m_Materials.emplace_back(std::make_shared<Material>((UINT)i, this));
		m_Materials.back()->LoadTextures(Scene->mMaterials[i]);
		m_Materials.back()->CreateConstantBuffer();
	}
}

void ModelData::RenderMeshes(const std::vector<std::unique_ptr<Mesh>>& Meshes)
{
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();

	for (const std::unique_ptr<Mesh>& m : Meshes)
	{
		std::shared_ptr<Material> Mat = m.get()->m_Material;

		DeviceContext->VSSetConstantBuffers(1u, 1u, m->m_pNode->m_ConstantBuffer.GetAddressOf());
		DeviceContext->PSSetConstantBuffers(1u, 1u, Mat->m_ConstantBuffer.GetAddressOf());

		if (Mat->m_DiffuseSRV >= 0)
		{
			DeviceContext->PSSetShaderResources(0u, 1u, &m_Textures[Mat->m_DiffuseSRV]);
		}

		if (Mat->m_SpecularSRV >= 0)
		{
			DeviceContext->PSSetShaderResources(1u, 1u, &m_Textures[Mat->m_SpecularSRV]);
		}

		//Graphics::GetSingletonPtr()->SetRasterStateBackFaceCull(!Mat->m_bTwoSided); // TODO: this doesn't actually work in some cases, investigate
		Graphics::GetSingletonPtr()->SetRasterStateBackFaceCull(true);
		//Graphics::GetSingletonPtr()->SetWireframeRasterState(); // swap back to line above when done or refactor to support switching

		// ensure the dispatch is finished before drawing

		DeviceContext->DrawIndexedInstancedIndirect(m->GetArgsBuffer().Get(), 0u);
		m_Profiler->AddDrawCall();
	}
}
