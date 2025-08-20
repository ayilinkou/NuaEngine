#pragma once

#ifndef FRUSTUM_CULLER_H
#define FRUSTUM_CULLER_H

#include <vector>
#include <memory>

#include "DirectXMath.h"
#include "d3d11.h"

#include "wrl.h"

class Profiler;
class ModelData;
struct AABB;
struct CullTransformData;

class FrustumCuller
{
private:
	struct CBufferData
	{
		UINT SentInstanceCount;
		UINT ThreadGroupCount[3];
		UINT GrassPerChunk;
		UINT PlaneDimension;
		float HeightDisplacement;
		float LODDistanceThreshold;
		DirectX::XMFLOAT3 Min;
		float Padding;
		DirectX::XMFLOAT3 Max;
		float Padding1;
	};

	struct InstanceCountMultiplierBufferData
	{
		UINT Multiplier;
		DirectX::XMFLOAT3 Padding;
	};

	struct GrassData
	{
		DirectX::XMFLOAT2 Offset;
		UINT ChunkID;
		UINT LOD;
	};

public:
	FrustumCuller() = default;
	~FrustumCuller();

	bool Init(std::shared_ptr<Profiler> pProfiler);
	void Shutdown();

	void DispatchShaderNew(ModelData* Model);
	void DispatchShader(const std::vector<CullTransformData>& Transforms, const AABB& BBox);
	void DispatchShader(const std::vector<DirectX::XMFLOAT2>& Offsets, const AABB& BBox);
	void CullGrass(ID3D11ShaderResourceView* GrassOffsetsSRV, const AABB& BBox, const UINT GrassPerChunk, const UINT VisibleChunkCount,
		UINT PlaneDimension, float HeightDisplacement, float LODDistanceThreshold, ID3D11ShaderResourceView* Heightmap);
	void ClearInstanceCount(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> InstanceCountUAV);
	void SendInstanceCounts(Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> FirstArgsBufferUAV, Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> SecondArgsBufferUAV = ms_DummyArgsBufferUAV);

	std::array<UINT, 2> GetInstanceCounts();

	Microsoft::WRL::ComPtr<ID3D11Buffer> GetCulledTransformsBuffer() const { return m_CulledTransformsBuffer; }
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetCulledOffsetsBuffer() const { return m_CulledOffsetsBuffer; }
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetCulledTransformsSRV() const { return m_CulledTransformsSRV; }
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetCulledOffsetsSRV() const { return m_CulledOffsetsSRV; }
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetCulledGrassDataSRV() const { return m_CulledGrassDataSRV; }
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetCulledGrassLODDataSRV() const { return m_CulledGrassLODDataSRV; }

private:
	bool CreateBuffers();
	bool CreateBufferViews();
	bool InitialiseStatics();

	void UpdateBuffers(const std::vector<CullTransformData>& Transforms, const AABB& BBox, UINT* ThreadGroupCount,
		UINT SentInstanceCount, UINT GrassPerChunk = 0u, UINT PlaneDimension = 0u, float HeightDisplacement = 0.f);
	void UpdateBuffers(const std::vector<DirectX::XMFLOAT2>& Offsets, const AABB& Corners, UINT* ThreadGroupCount,
		UINT SentInstanceCount, UINT GrassPerChunk = 0u, UINT PlaneDimension = 0u, float HeightDisplacement = 0.f);
	void UpdateCBuffer(const AABB& BBox, UINT* ThreadGroupCount, UINT SentInstanceCount, UINT GrassPerChunk = 0u,
		UINT PlaneDimension = 0u, float HeightDisplacement = 0.f, float LODDistanceThreshold = 0.f);

	void DispatchShaderImpl(UINT* ThreadGroupCount);
	void DispatchShaderImplNew(ModelData* Model, UINT* ThreadGroupCount);

	void StoreInstanceCount(ModelData* pModel);
	void SendInstanceCountsNew(ModelData* pModel);

private:
	ID3D11ComputeShader* m_CullingShader					= nullptr;
	ID3D11ComputeShader* m_OffsetsCullingShader				= nullptr;
	ID3D11ComputeShader* m_GrassCullingShader				= nullptr;
	ID3D11ComputeShader* m_InstanceCountClearShader			= nullptr;
	ID3D11ComputeShader* m_InstanceCountTransferShader		= nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_TransformsBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_OffsetsBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_CulledTransformsBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_CulledOffsetsBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_CulledGrassDataBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_CulledGrassLODDataBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_CBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_StagingBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_InstanceCountBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_TransformsSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_OffsetsSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_CulledTransformsSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_CulledOffsetsSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_CulledGrassDataSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_CulledGrassLODDataSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_InstanceCountBufferUAV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_CulledTransformsUAV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_CulledOffsetsUAV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_CulledGrassDataUAV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_CulledGrassLODDataUAV;

	static Microsoft::WRL::ComPtr<ID3D11Buffer> ms_DummyArgsBuffer;
	static Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> ms_DummyArgsBufferUAV;
	static bool ms_bStaticsInitialised;

	const char* m_csFilename = "";
	bool m_bGotInstanceCount = false;

	std::shared_ptr<Profiler> m_Profiler;
};

#endif
