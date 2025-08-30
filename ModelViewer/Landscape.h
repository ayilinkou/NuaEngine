#pragma once

#ifndef LANDSCAPE_H
#define LANDSCAPE_H

#include "d3d11.h"
#include "DirectXMath.h"

#include "wrl.h"

#include "GameObject.h"
#include "AABB.h"
#include "CullData.h"

class CameraManager;

class Landscape : public GameObject
{
	friend class TessellatedPlane;
	friend class Grass;

private:	
	struct LandscapeInfoCBuffer
	{
		float PlaneDimension;
		float HeightDisplacement;
		UINT ChunkInstanceCount;
		BOOL bVisualiseChunks;
		DirectX::XMMATRIX ChunkScaleMatrix;
		UINT GrassPerChunk;
		float PlaneSpecular;
		float GrassSpecular;
		float Padding;
	};

public:
	Landscape() = delete;
	Landscape(UINT NumChunks, float ChunkSize, float HeightDisplacement, std::shared_ptr<CameraManager> CamManager);
	~Landscape();

	bool Init(const std::string& HeightMapFilepath, float TessellationScale, UINT GrassDimensionPerChunk);
	void Render();
	void Shutdown();

	virtual void RenderControls() override;

	void SetShouldRender(bool bNewShouldRender) { m_bShouldRender = bNewShouldRender; }
	bool ShouldRender() const { return m_bShouldRender; }
	bool GetShouldRenderBBoxes() const { return m_bShouldRenderBBoxes; }

	float GetHeightDisplacement() const { return m_HeightDisplacement; }
	void SetHeightDisplacement(float NewHeight);
	std::vector<DirectX::XMFLOAT2>& GetChunkOffsets() { return m_ChunkOffsets; }
	std::vector<DirectX::XMFLOAT2>& GetGrassOffsets() { return m_GrassOffsets; }
	std::vector<CullTransformData>& GetChunkTransforms() { return m_ChunkTransforms; }
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetCulledTransformsSRV() { return m_CulledTransformsSRV; }
	UINT GetChunkInstanceCount() const { return m_ChunkInstanceCount; }
	UINT GetChunkDimension() const { return m_ChunkDimension; }

	std::shared_ptr<TessellatedPlane> GetPlane() { return m_Plane; }
	std::shared_ptr<Grass> GetGrass() { return m_Grass; }

	AABB& GetBoundingBox() { return m_BoundingBox; }

	ID3D11ShaderResourceView* GetHeightmapSRV() const { return m_HeightmapSRV; }

private:
	bool CreateBuffers();
	void SetupAABB();

	void UpdateBuffers();

	void GenerateChunkOffsets();
	void GenerateGrassOffsets(UINT GrassCount);

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_LandscapeInfoCBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ChunkOffsetsBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_ChunkOffsetsSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_CulledTransformsSRV;

	std::shared_ptr<TessellatedPlane> m_Plane;
	std::shared_ptr<Grass> m_Grass;
	std::vector<DirectX::XMFLOAT2> m_ChunkOffsets;
	std::vector<DirectX::XMFLOAT2> m_GrassOffsets;
	std::vector<CullTransformData> m_ChunkTransforms;

	AABB m_BoundingBox;
	std::unique_ptr<CullData> m_PlaneCullData;
	std::vector<ID3D11UnorderedAccessView*> m_ArgsBufferUAVs;

	ID3D11ShaderResourceView* m_HeightmapSRV;
	std::string m_HeightMapFilepath;

	bool m_bShouldRender;
	bool m_bVisualiseChunks;
	bool m_bShouldRenderBBoxes;
	float m_ChunkSize;
	float m_HeightDisplacement;
	UINT m_ChunkDimension;
	UINT m_NumChunks;
	UINT m_ChunkInstanceCount;
	float m_PlaneSpecular;
	float m_GrassSpecular;

	std::shared_ptr<CameraManager> m_CameraManager;
};

#endif
