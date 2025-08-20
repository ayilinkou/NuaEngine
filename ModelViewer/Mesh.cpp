#include "assimp/scene.h"

#include "Mesh.h"
#include "ModelData.h"
#include "Node.h"
#include "Graphics.h"
#include "MyMacros.h"

Mesh::Mesh(ModelData* pModel, Node* pNode) : m_pModel(pModel), m_pNode(pNode)
{
}

void Mesh::ProcessMesh(aiMesh* SceneMesh)
{
	// TODO: this is duplicating vertices, come back to this and optimise at some point
	
	m_Name = SceneMesh->mName.C_Str();
	m_VerticesOffset = (UINT)m_pModel->GetVertices().size();
	m_IndicesOffset = (UINT)m_pModel->GetIndices().size();
	m_Material = m_pModel->GetMaterials()[SceneMesh->mMaterialIndex];

	for (size_t i = 0; i < SceneMesh->mNumVertices; i++)
	{
		Vertex v;
		v.Pos = DirectX::XMFLOAT3(SceneMesh->mVertices[i].x, SceneMesh->mVertices[i].y, SceneMesh->mVertices[i].z);
		v.Normal = DirectX::XMFLOAT3(SceneMesh->mNormals[i].x, SceneMesh->mNormals[i].y, SceneMesh->mNormals[i].z);

		if (SceneMesh->mTextureCoords[0])
		{
			v.TexCoord = DirectX::XMFLOAT2(SceneMesh->mTextureCoords[0][i].x, SceneMesh->mTextureCoords[0][i].y);
		}
		else
		{
			v.TexCoord = DirectX::XMFLOAT2(0.f, 0.f);
		}

		UpdateBoundingBox(v.Pos);
		m_pModel->GetVertices().push_back(v);
	}
	m_VertexCount = (UINT)m_pModel->GetVertices().size() - m_VerticesOffset;

	for (size_t i = 0; i < SceneMesh->mNumFaces; i++)
	{
		aiFace Face = SceneMesh->mFaces[i];
		for (size_t j = 0; j < Face.mNumIndices; j++)
		{
			m_pModel->GetIndices().push_back(Face.mIndices[j] + m_VerticesOffset);
		}
	}
	m_IndexCount = (UINT)m_pModel->GetIndices().size() - m_IndicesOffset;

	bool bResult = CreateArgsBuffer();
	assert(bResult);
}

bool Mesh::CreateArgsBuffer()
{
	HRESULT hResult;
	D3D11_BUFFER_DESC abDesc = {};
	abDesc.ByteWidth = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS);
	abDesc.Usage = D3D11_USAGE_DEFAULT;
	abDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	abDesc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS | D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

	D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS ArgsData;
	ArgsData.IndexCountPerInstance = m_IndexCount;
	ArgsData.InstanceCount = 0u;
	ArgsData.StartIndexLocation = m_IndicesOffset;
	ArgsData.BaseVertexLocation = 0;
	ArgsData.StartInstanceLocation = 0u;

	D3D11_SUBRESOURCE_DATA Data = {};
	Data.pSysMem = &ArgsData;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&abDesc, &Data, &m_ArgsBuffer));
	NAME_D3D_RESOURCE(m_ArgsBuffer, (m_pModel->GetModelPath() + " " + m_Name + " args buffer").c_str());

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
	uavDesc.Buffer.NumElements = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS) / 4;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateUnorderedAccessView(m_ArgsBuffer.Get(), &uavDesc, &m_ArgsBufferUAV));
	NAME_D3D_RESOURCE(m_ArgsBufferUAV, (m_pModel->GetModelPath() + " " + m_Name + " args buffer UAV").c_str());
	
	m_pModel->GetArgsBufferUAVs().push_back(m_ArgsBufferUAV.Get());
	return true;
}

void Mesh::UpdateBoundingBox(DirectX::XMFLOAT3 Pos)
{
	AABB& BBox = m_pModel->GetBoundingBox();

	DirectX::XMVECTOR PosVec = DirectX::XMVectorSet(Pos.x, Pos.y, Pos.z, 1.f);
	PosVec = DirectX::XMVector3Transform(PosVec, m_pNode->GetAccumulatedTransform());
	DirectX::XMFLOAT3 TransformedPos;
	DirectX::XMStoreFloat3(&TransformedPos, PosVec);

	BBox.Expand(TransformedPos);
}
