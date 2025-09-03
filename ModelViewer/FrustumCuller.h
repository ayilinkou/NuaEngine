#pragma once

#ifndef FRUSTUM_CULLER_H
#define FRUSTUM_CULLER_H

#include <vector>
#include <memory>
#include <array>

#include "DirectXMath.h"
#include "d3d11.h"

#include "wrl.h"

class Profiler;
class CullData;
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

	void DispatchShaderNew(CullData* Data);
	void CullGrass(CullData& Data, const UINT GrassPerChunk, const UINT VisibleChunkCount,
		UINT PlaneDimension, float HeightDisplacement, float LODDistanceThreshold, ID3D11ShaderResourceView* Heightmap,
		ID3D11ShaderResourceView* CulledTransformsSRV, ID3D11ShaderResourceView* GrassOffsetsSRV);
	void ClearInstanceCount();
	void SendInstanceCounts(Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> FirstArgsBufferUAV, Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> SecondArgsBufferUAV = ms_DummyArgsBufferUAV);

	std::array<UINT, 2> GetInstanceCounts();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetCulledGrassDataSRV() const { return m_CulledGrassDataSRV; }
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetCulledGrassLODDataSRV() const { return m_CulledGrassLODDataSRV; }

private:
	bool CreateBuffers();
	bool CreateBufferViews();
	bool InitialiseStatics();

	void UpdateCBuffer(const AABB* BBox, UINT* ThreadGroupCount, UINT SentInstanceCount, UINT GrassPerChunk = 0u,
		UINT PlaneDimension = 0u, float HeightDisplacement = 0.f, float LODDistanceThreshold = 0.f);

	void DispatchShaderImplNew(ID3D11ShaderResourceView* TransformsSRV, ID3D11UnorderedAccessView* CulledTransformsUAV,
		UINT* ThreadGroupCount);

	void StoreInstanceCounts(std::array<UINT*, 2> OutInstanceCounts);
	void SendInstanceCountsNew(std::vector<ID3D11UnorderedAccessView*>* ArgsBufferUAVs);

private:
	ID3D11ComputeShader* m_CullingShader					= nullptr;
	ID3D11ComputeShader* m_GrassCullingShader				= nullptr;
	ID3D11ComputeShader* m_InstanceCountClearShader			= nullptr;
	ID3D11ComputeShader* m_InstanceCountTransferShader		= nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_CulledGrassDataBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_CulledGrassLODDataBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_CBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_StagingBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_InstanceCountBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_CulledGrassDataSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_CulledGrassLODDataSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_InstanceCountBufferUAV;
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
