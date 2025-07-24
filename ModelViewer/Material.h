#pragma once

#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>

#include "DirectXMath.h"

#include "d3d11.h"

#include "wrl.h"

struct aiMaterial;

struct MaterialData
{
	DirectX::XMFLOAT3 DiffuseColor = { 1.f, 1.f, 1.f };
	int DiffuseSRV = -1;
	DirectX::XMFLOAT3 Specular = { 1.f, 1.f, 1.f };
	int SpecularSRV = -1;
};

class Material
{
	friend class ModelData;
	friend class Mesh;
	friend class Node;

public:
	Material(UINT Index, ModelData* pOwner);

	void LoadTextures(aiMaterial* MeshMat);
	void LoadTexture(const std::string& Path, int& TextureIndex);
	void CreateConstantBuffer();

private:
	DirectX::XMFLOAT3 m_DiffuseColor = {};
	DirectX::XMFLOAT3 m_Specular = {};
	int m_DiffuseSRV = -1;
	int m_SpecularSRV = -1;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ConstantBuffer;
	bool m_bTwoSided = true;
	bool m_bOpaque = false;

	std::string m_Name;
	
	UINT m_uIndex;
	ModelData* m_pOwner;

};

#endif
