#pragma once

#ifndef MODEL_DATA_H
#define MODEL_DATA_H

#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "wrl.h"

#include "d3d11.h"
#include "DirectXMath.h"

#include "CullData.h"
#include "AABB.h"

class Mesh;
class Material;
class Node;
class FrustumCuller;
class Profiler;
struct aiScene;
struct Vertex;

class ModelData
{
public:
	ModelData() = delete;
	ModelData(const std::string& ModelPath, FrustumCuller* pFrustumCuller, std::shared_ptr<Profiler> pProfiler,
		const std::string& TexturesPath = "");
	ModelData(const ModelData& Other) = delete;
	~ModelData();

	bool Initialise(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext, const std::string& ModelFilename, const std::string& TexturesPath);
	void Shutdown();

	void RenderOpaque();
	void RenderTransparent();

	CullData* GetCullData() { return m_CullData.get(); }
	UINT GetInstanceCount() const { return m_InstanceCount; }

	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer() const { return m_VertexBuffer; }
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer() const { return m_IndexBuffer; }
	std::vector<Vertex>& GetVertices() { return m_Vertices; }
	std::vector<UINT>& GetIndices() { return m_Indices; }
	std::vector<Mesh*>& GetMeshes() { return m_Meshes; }
	std::vector<std::unique_ptr<Mesh>>& GetOpaqueMeshes() { return m_OpaqueMeshes; }
	std::vector<std::unique_ptr<Mesh>>& GetTransparentMeshes() { return m_TransparentMeshes; }
	std::vector<std::shared_ptr<Material>>& GetMaterials() { return m_Materials; }
	std::vector<ID3D11ShaderResourceView*>& GetTextures() { return m_Textures; }
	std::vector<ID3D11UnorderedAccessView*>& GetArgsBufferUAVs() { return m_ArgsBufferUAVs; }
	std::unordered_map<std::string, UINT>& GetTextureIndexMap() { return m_TextureIndexMap; }
	std::unordered_set<std::string>& GetTexturePathsSet() { return m_TexturePathsSet; }

	std::vector<CullTransformData>& GetTransforms() { return m_Transforms; }
	AABB& GetBoundingBox() { return m_BoundingBox; }

	std::string GetModelPath() const { return m_ModelPath; }
	std::string GetTexturesPath() const { return m_TexturesPath; }

	void UpdateBuffers();

private:
	void ShutdownBuffers();

	bool LoadModel();
	void ReleaseModel();
	void Reset();

	bool CreateBuffers();
	void LoadMaterials(const aiScene* Scene);

	void RenderMeshes(const std::vector<std::unique_ptr<Mesh>>& Meshes);

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_VertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_IndexBuffer;
	std::vector<Vertex> m_Vertices;
	std::vector<UINT> m_Indices;
	std::vector<Mesh*> m_Meshes;
	std::vector<std::unique_ptr<Mesh>> m_OpaqueMeshes;
	std::vector<std::unique_ptr<Mesh>> m_TransparentMeshes;
	std::unique_ptr<Node> m_RootNode;
	std::vector<std::shared_ptr<Material>> m_Materials;
	std::vector<ID3D11ShaderResourceView*> m_Textures;
	std::unordered_map<std::string, UINT> m_TextureIndexMap;
	std::unordered_set<std::string> m_TexturePathsSet;

	std::unique_ptr<CullData> m_CullData;
	std::vector<ID3D11UnorderedAccessView*> m_ArgsBufferUAVs;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_CulledTransformsSRV;

	std::vector<CullTransformData> m_Transforms;
	AABB m_BoundingBox;
	UINT m_InstanceCount;
	
	std::string m_ModelPath;
	std::string m_TexturesPath;

	FrustumCuller* m_FrustumCuller;
	std::shared_ptr<Profiler> m_Profiler;

};

#endif
