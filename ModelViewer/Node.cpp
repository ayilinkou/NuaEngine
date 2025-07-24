#include "assimp/scene.h"

#include "Node.h"
#include "ModelData.h"
#include "Graphics.h"
#include "MyMacros.h"
#include "Material.h"

Node::Node(ModelData* pModel, Node* pOwner) : m_pModel(pModel), m_pOwner(pOwner)
{
	m_LocalTransform = DirectX::XMMatrixIdentity();
	m_AccumulatedTransform = DirectX::XMMatrixIdentity();
}

void Node::ProcessNode(aiNode* ModelNode, const aiScene* Scene, const DirectX::XMMATRIX& AccumulatedTransform)
{
	m_NodeName = ModelNode->mName.C_Str();
	
	m_LocalTransform = ConvertToXMMATRIX(ModelNode->mTransformation);
	m_AccumulatedTransform = AccumulatedTransform * m_LocalTransform;
	CreateConstantBuffer();
	
	for (size_t i = 0; i < ModelNode->mNumMeshes; i++)
	{
		aiMesh* SceneMesh = Scene->mMeshes[ModelNode->mMeshes[i]];
		bool bOpaque = m_pModel->GetMaterials()[SceneMesh->mMaterialIndex]->m_bOpaque;

		if (bOpaque)
		{
			m_pModel->GetOpaqueMeshes().emplace_back(std::make_unique<Mesh>(m_pModel, this));
			m_pModel->GetOpaqueMeshes().back()->ProcessMesh(SceneMesh);
		}
		else
		{
			m_pModel->GetTransparentMeshes().emplace_back(std::make_unique<Mesh>(m_pModel, this));
			m_pModel->GetTransparentMeshes().back()->ProcessMesh(SceneMesh);
		}
	}

	for (size_t i = 0; i < ModelNode->mNumChildren; i++)
	{
		m_Children.emplace_back(std::make_unique<Node>(m_pModel, this));
		m_Children.back().get()->ProcessNode(ModelNode->mChildren[i], Scene, m_AccumulatedTransform);
	}
}

DirectX::XMMATRIX Node::ConvertToXMMATRIX(const aiMatrix4x4& aiMatrix) const
{
	return DirectX::XMMATRIX(
		aiMatrix.a1, aiMatrix.a2, aiMatrix.a3, aiMatrix.a4,
		aiMatrix.b1, aiMatrix.b2, aiMatrix.b3, aiMatrix.b4,
		aiMatrix.c1, aiMatrix.c2, aiMatrix.c3, aiMatrix.c4,
		aiMatrix.d1, aiMatrix.d2, aiMatrix.d3, aiMatrix.d4
	);
}

void Node::CreateConstantBuffer()
{
	HRESULT hResult;
	D3D11_BUFFER_DESC BufferDesc = {};
	BufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	BufferDesc.ByteWidth = sizeof(DirectX::XMMATRIX);
	BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	D3D11_SUBRESOURCE_DATA Data;
	Data.pSysMem = &m_AccumulatedTransform;

	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &Data, &m_ConstantBuffer));
	NAME_D3D_RESOURCE(m_ConstantBuffer, (m_pModel->GetModelPath() + " " + m_NodeName + " constant buffer").c_str());
}
