#include "Material.h"
#include "Graphics.h"
#include "MyMacros.h"

#include "assimp/material.h"

#include "ResourceManager.h"
#include "ModelData.h"

Material::Material(UINT Index, ModelData* pOwner) : m_uIndex(Index), m_pOwner(pOwner)
{
}

void Material::LoadTextures(aiMaterial* MeshMat)
{
	m_Name = MeshMat->GetName().C_Str();

	MeshMat->Get(AI_MATKEY_TWOSIDED, m_bTwoSided);
	float Opacity = 0.f;
	MeshMat->Get(AI_MATKEY_OPACITY, Opacity);
	m_bOpaque = Opacity >= 1.f;
	
	if (m_pOwner->GetTexturesPath().empty())
	{
		return;
	}

	aiString Path;
	aiColor3D Color;
	if (MeshMat->GetTexture(aiTextureType_DIFFUSE, 0, &Path) == AI_SUCCESS)
	{
		std::string FullPath = m_pOwner->GetTexturesPath() + std::string(Path.C_Str());
		LoadTexture(FullPath, m_DiffuseSRV);
	}
	else if (MeshMat->Get(AI_MATKEY_COLOR_DIFFUSE, Color) == AI_SUCCESS)
	{
		m_DiffuseColor.x = Color.r;
		m_DiffuseColor.y = Color.g;
		m_DiffuseColor.z = Color.b;
	}

	if (MeshMat->GetTexture(aiTextureType_SPECULAR, 0, &Path) == AI_SUCCESS)
	{
		std::string FullPath = m_pOwner->GetTexturesPath() + std::string(Path.C_Str());
		LoadTexture(FullPath, m_SpecularSRV);
	}
	else if (MeshMat->Get(AI_MATKEY_COLOR_SPECULAR, Color) == AI_SUCCESS)
	{
		m_Specular = Color.r;
	}
}

void Material::LoadTexture(const std::string& Path, int& TextureIndex)
{
	m_pOwner->GetTexturePathsSet().insert(Path);
	std::unordered_map<std::string, UINT>& TextureIndexMap = m_pOwner->GetTextureIndexMap();
	std::vector<ID3D11ShaderResourceView*>& Textures = m_pOwner->GetTextures();
	if (TextureIndexMap.find(Path) == TextureIndexMap.end())
	{
		// not loaded, load and add index to map
		ResourceManager* pResManager = ResourceManager::GetSingletonPtr();

		ID3D11ShaderResourceView* SRV = static_cast<ID3D11ShaderResourceView*>(pResManager->LoadTexture(Path.c_str()));
		Textures.push_back(SRV);

		UINT Index = (UINT)Textures.size() - 1;
		TextureIndexMap.insert({ Path, Index });

		TextureIndex = Index;
	}
	else
	{
		// already loaded, find index and assign
		UINT Index = TextureIndexMap.at(Path);
		TextureIndex = Index;
	}
}

void Material::CreateConstantBuffer()
{
	HRESULT hResult;
	D3D11_BUFFER_DESC BufferDesc = {};
	BufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	BufferDesc.ByteWidth = sizeof(MaterialData);
	BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	MaterialData Data = {};
	Data.DiffuseColor = m_DiffuseColor;
	Data.DiffuseSRV = m_DiffuseSRV;
	Data.SpecularSRV = m_SpecularSRV;
	Data.Specular = m_Specular;

	D3D11_SUBRESOURCE_DATA BufferData = {};
	BufferData.pSysMem = &Data;

	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &BufferData, &m_ConstantBuffer));
	NAME_D3D_RESOURCE(m_ConstantBuffer, (m_pOwner->GetModelPath() + " " + m_Name + " constant buffer").c_str());
}
