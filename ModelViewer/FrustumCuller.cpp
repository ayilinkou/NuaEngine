#include <cstdlib>
#include <cwchar>
#include <cstring>
#include <fstream>
#include <array>

#include "d3dcompiler.h"

#include "FrustumCuller.h"
#include "Graphics.h"
#include "MyMacros.h"
#include "Common.h"
#include "ResourceManager.h"
#include "Camera.h"
#include "Profiler.h"
#include "AABB.h"
#include "CullData.h"

bool FrustumCuller::ms_bStaticsInitialised = false;
Microsoft::WRL::ComPtr<ID3D11Buffer> FrustumCuller::ms_DummyArgsBuffer;
Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> FrustumCuller::ms_DummyArgsBufferUAV;

FrustumCuller::~FrustumCuller()
{
	Shutdown();
}

bool FrustumCuller::Init(std::shared_ptr<Profiler> pProfiler)
{
	Microsoft::WRL::ComPtr<ID3D10Blob> csBuffer;
	m_csFilename = "Shaders/FrustumCullingCS.hlsl";
	m_bGotInstanceCount = false;
	m_Profiler = pProfiler;

	m_CullingShader							= ResourceManager::GetSingletonPtr()->LoadShader<ID3D11ComputeShader>(m_csFilename, "FrustumCull");
	m_GrassCullingShader					= ResourceManager::GetSingletonPtr()->LoadShader<ID3D11ComputeShader>(m_csFilename, "FrustumCullGrass");
	m_InstanceCountClearShader				= ResourceManager::GetSingletonPtr()->LoadShader<ID3D11ComputeShader>(m_csFilename, "ClearInstanceCount");
	m_InstanceCountTransferShader			= ResourceManager::GetSingletonPtr()->LoadShader<ID3D11ComputeShader>(m_csFilename, "TransferInstanceCount");

	bool Result;
	FALSE_IF_FAILED(CreateBuffers());
	FALSE_IF_FAILED(CreateBufferViews());
	FALSE_IF_FAILED(InitialiseStatics());

	return true;
}

void FrustumCuller::Shutdown()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11ComputeShader>(m_csFilename, "FrustumCull");
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11ComputeShader>(m_csFilename, "FrustumCullGrass");
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11ComputeShader>(m_csFilename, "ClearInstanceCount");
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11ComputeShader>(m_csFilename, "TransferInstanceCount");
}

std::array<UINT, 2> FrustumCuller::GetInstanceCounts()
{
	HRESULT hResult;
	D3D11_MAPPED_SUBRESOURCE Data = {};
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();

	DeviceContext->CopyResource(m_StagingBuffer.Get(), m_InstanceCountBuffer.Get());
	
	std::array<UINT, 2> InstanceCounts;
	ASSERT_NOT_FAILED(DeviceContext->Map(m_StagingBuffer.Get(), 0u, D3D11_MAP_READ, 0u, &Data));
	memcpy(InstanceCounts.data(), Data.pData, sizeof(UINT) * 2);
	DeviceContext->Unmap(m_StagingBuffer.Get(), 0u);

	assert(!m_bGotInstanceCount && "Should avoid calling this more than once for the same culling dispatch. If have to, store the value when calling first time.");
	m_bGotInstanceCount = true;

	return InstanceCounts;
}

void FrustumCuller::DispatchShaderNew(CullData* Data)
{
	ClearInstanceCount();

	// As each thread group will have 32 threads (as defined in shader), calculate how many thread groups we need using integer division
	UINT ThreadGroupCount[3] = { Data->GetSentInstanceCount() + 31u / 32u, 1u, 1u};

	UpdateCBuffer(Data->GetBoundingBox(), ThreadGroupCount, Data->GetSentInstanceCount());
	DispatchShaderImplNew(Data->GetTransformsSRV(), Data->GetCulledTransformsUAV(), ThreadGroupCount);
	StoreInstanceCounts(Data->GetOutInstanceCounts());
	SendInstanceCountsNew(Data->GetArgsBufferUAVs());
}

void FrustumCuller::DispatchShader(const std::vector<CullTransformData>& Transforms, const AABB& BBox)
{
	ClearInstanceCount();
	Graphics::GetSingletonPtr()->GetDeviceContext()->CSSetShader(m_CullingShader, nullptr, 0u);

	// As each thread group will have 32 threads (as defined in shader), calculate how many thread groups we need using integer division
	UINT ThreadGroupCount[3] = { (UINT(Transforms.size()) + 31) / 32u, 1u, 1u };

	UpdateBuffers(Transforms, BBox, ThreadGroupCount, (UINT)Transforms.size());
	DispatchShaderImpl(ThreadGroupCount);
}

void FrustumCuller::CullGrass(CullData& Data, const UINT GrassPerChunk, const UINT VisibleChunkCount,
	UINT PlaneDimension, float HeightDisplacement, float LODDistanceThreshold, ID3D11ShaderResourceView* Heightmap,
	ID3D11ShaderResourceView* CulledTransformsSRV, ID3D11ShaderResourceView* GrassOffsetsSRV)
{
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();
	const UINT InitialCount = 0u;

	ClearInstanceCount();
	DeviceContext->CSSetShader(m_GrassCullingShader, nullptr, 0u);

	const UINT ThreadsX = 32u;
	const UINT ThreadsY = 8u;
	const UINT DispatchX = (GrassPerChunk + ThreadsX - 1) / ThreadsX;
	const UINT DispatchY = (VisibleChunkCount + ThreadsY - 1) / ThreadsY;
	UINT ThreadGroupCount[3] = { DispatchX, DispatchY, 1u };

	UpdateCBuffer(Data.GetBoundingBox(), ThreadGroupCount, VisibleChunkCount, GrassPerChunk, PlaneDimension, HeightDisplacement, LODDistanceThreshold);

	ID3D11ShaderResourceView* SRVs[] = { CulledTransformsSRV, GrassOffsetsSRV, Heightmap};
	DeviceContext->CSSetUnorderedAccessViews(2u, 1u, m_CulledGrassDataUAV.GetAddressOf(), &InitialCount);
	DeviceContext->CSSetUnorderedAccessViews(3u, 1u, m_CulledGrassLODDataUAV.GetAddressOf(), &InitialCount);
	DeviceContext->CSSetUnorderedAccessViews(4u, 1u, m_InstanceCountBufferUAV.GetAddressOf(), nullptr);
	DeviceContext->CSSetShaderResources(0u, 3u, SRVs);
	DeviceContext->CSSetConstantBuffers(1u, 1u, m_CBuffer.GetAddressOf());

	DeviceContext->Dispatch(ThreadGroupCount[0], ThreadGroupCount[1], ThreadGroupCount[2]);
	m_Profiler->AddComputeDispatch();

	DeviceContext->CSSetConstantBuffers(1u, 7u, NullBuffers);
	DeviceContext->CSSetShaderResources(0u, 8u, NullSRVs);
	DeviceContext->CSSetUnorderedAccessViews(0u, 8u, NullUAVs, nullptr);
	DeviceContext->CSSetShader(nullptr, nullptr, 0u);

	StoreInstanceCounts(Data.GetOutInstanceCounts());
	auto& UAVs = *Data.GetArgsBufferUAVs();
	SendInstanceCounts(UAVs[0], UAVs[1]);
}

void FrustumCuller::ClearInstanceCount()
{
	Graphics::GetSingletonPtr()->GetDeviceContext()->CSSetShader(m_InstanceCountClearShader, nullptr, 0u);
	Graphics::GetSingletonPtr()->GetDeviceContext()->CSSetUnorderedAccessViews(4u, 1u, m_InstanceCountBufferUAV.GetAddressOf(), nullptr);
	Graphics::GetSingletonPtr()->GetDeviceContext()->Dispatch(1u, 1u, 1u);
	m_Profiler->AddComputeDispatch();
	m_bGotInstanceCount = false;
}

void FrustumCuller::SendInstanceCounts(Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> FirstArgsBufferUAV, Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> SecondArgsBufferUAV)
{
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();
	
	ID3D11UnorderedAccessView* UAVs[] = { m_InstanceCountBufferUAV.Get(), FirstArgsBufferUAV.Get(), SecondArgsBufferUAV.Get() };
	DeviceContext->CSSetShader(m_InstanceCountTransferShader, nullptr, 0u);
	DeviceContext->CSSetUnorderedAccessViews(4u, 3u, UAVs, nullptr);

	DeviceContext->Dispatch(1u, 1u, 1u);
	m_Profiler->AddComputeDispatch();

	DeviceContext->CSSetShader(nullptr, nullptr, 0u);
	DeviceContext->CSSetUnorderedAccessViews(0u, 8u, NullUAVs, nullptr);
}

bool FrustumCuller::CreateBuffers()
{
	HRESULT hResult;
	D3D11_BUFFER_DESC Desc = {};
	ID3D11Device* Device = Graphics::GetSingletonPtr()->GetDevice();

	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.ByteWidth = (UINT)(sizeof(CullTransformData) * MAX_INSTANCE_COUNT);
	Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	Desc.StructureByteStride = sizeof(CullTransformData);

	HFALSE_IF_FAILED(Device->CreateBuffer(&Desc, nullptr, &m_TransformsBuffer));
	NAME_D3D_RESOURCE(m_TransformsBuffer, "Frustum culler transforms buffer");

	Desc.CPUAccessFlags = 0;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

	HFALSE_IF_FAILED(Device->CreateBuffer(&Desc, nullptr, &m_CulledTransformsBuffer));
	NAME_D3D_RESOURCE(m_CulledTransformsBuffer, "Frustum culler culled transforms buffer");

	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.ByteWidth = (UINT)(sizeof(DirectX::XMFLOAT2) * MAX_GRASS_PER_CHUNK);
	Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	Desc.StructureByteStride = sizeof(DirectX::XMFLOAT2);

	HFALSE_IF_FAILED(Device->CreateBuffer(&Desc, nullptr, &m_OffsetsBuffer));
	NAME_D3D_RESOURCE(m_OffsetsBuffer, "Frustum culler offsets buffer");

	Desc.CPUAccessFlags = 0;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.ByteWidth = (UINT)(sizeof(DirectX::XMFLOAT2) * MAX_INSTANCE_COUNT);
	Desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

	HFALSE_IF_FAILED(Device->CreateBuffer(&Desc, nullptr, &m_CulledOffsetsBuffer));
	NAME_D3D_RESOURCE(m_CulledOffsetsBuffer, "Frustum culler culled offsets buffer");

	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.ByteWidth = (UINT)(sizeof(GrassData) * MAX_GRASS_COUNT);
	Desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	Desc.StructureByteStride = sizeof(GrassData);

	HFALSE_IF_FAILED(Device->CreateBuffer(&Desc, nullptr, &m_CulledGrassDataBuffer));
	NAME_D3D_RESOURCE(m_CulledGrassDataBuffer, "Frustum culler culled grass data buffer");

	HFALSE_IF_FAILED(Device->CreateBuffer(&Desc, nullptr, &m_CulledGrassLODDataBuffer));
	NAME_D3D_RESOURCE(m_CulledGrassLODDataBuffer, "Frustum culler culled grass LOD data buffer");

	Desc = {};
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.ByteWidth = sizeof(CBufferData);
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HFALSE_IF_FAILED(Device->CreateBuffer(&Desc, nullptr, &m_CBuffer));
	NAME_D3D_RESOURCE(m_CBuffer, "Frustum culler constant buffer");

	Desc = {};
	Desc.Usage = D3D11_USAGE_STAGING;
	Desc.ByteWidth = sizeof(UINT) * 2;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

	HFALSE_IF_FAILED(Device->CreateBuffer(&Desc, nullptr, &m_StagingBuffer));
	NAME_D3D_RESOURCE(m_StagingBuffer, "Frustum culler staging buffer");

	D3D11_BUFFER_DESC InstanceBufferDesc = {};
	InstanceBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	InstanceBufferDesc.ByteWidth = sizeof(UINT) * 2;
	InstanceBufferDesc.StructureByteStride = sizeof(UINT);
	InstanceBufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	InstanceBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	HFALSE_IF_FAILED(Device->CreateBuffer(&InstanceBufferDesc, nullptr, &m_InstanceCountBuffer));
	NAME_D3D_RESOURCE(m_InstanceCountBuffer, "Frustum culler instance count buffer");

	return true;
}

bool FrustumCuller::CreateBufferViews()
{
	HRESULT hResult;
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	ID3D11Device* Device = Graphics::GetSingletonPtr()->GetDevice();

	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.NumElements = (UINT)MAX_INSTANCE_COUNT;
	uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;

	HFALSE_IF_FAILED(Device->CreateUnorderedAccessView(m_CulledTransformsBuffer.Get(), &uavDesc, &m_CulledTransformsUAV));
	NAME_D3D_RESOURCE(m_CulledTransformsUAV, "Frustum culler culled transforms buffer UAV");

	HFALSE_IF_FAILED(Device->CreateUnorderedAccessView(m_CulledOffsetsBuffer.Get(), &uavDesc, &m_CulledOffsetsUAV));
	NAME_D3D_RESOURCE(m_CulledOffsetsUAV, "Frustum culler culled offsets buffer UAV");

	uavDesc.Buffer.NumElements = (UINT)MAX_GRASS_COUNT;

	HFALSE_IF_FAILED(Device->CreateUnorderedAccessView(m_CulledGrassDataBuffer.Get(), &uavDesc, &m_CulledGrassDataUAV));
	NAME_D3D_RESOURCE(m_CulledGrassDataUAV, "Frustum culler culled grass data buffer UAV");

	HFALSE_IF_FAILED(Device->CreateUnorderedAccessView(m_CulledGrassLODDataBuffer.Get(), &uavDesc, &m_CulledGrassLODDataUAV));
	NAME_D3D_RESOURCE(m_CulledGrassLODDataUAV, "Frustum culler culled grass LOD data buffer UAV");

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.Buffer.NumElements = (UINT)MAX_INSTANCE_COUNT;

	HFALSE_IF_FAILED(Device->CreateShaderResourceView(m_TransformsBuffer.Get(), &SRVDesc, &m_TransformsSRV));
	NAME_D3D_RESOURCE(m_TransformsSRV, "Frustum culler transforms buffer SRV");

	HFALSE_IF_FAILED(Device->CreateShaderResourceView(m_CulledTransformsBuffer.Get(), &SRVDesc, &m_CulledTransformsSRV));
	NAME_D3D_RESOURCE(m_CulledTransformsSRV, "Frustum culler culled transforms buffer SRV");

	HFALSE_IF_FAILED(Device->CreateShaderResourceView(m_CulledOffsetsBuffer.Get(), &SRVDesc, &m_CulledOffsetsSRV));
	NAME_D3D_RESOURCE(m_CulledOffsetsSRV, "Frustum culler culled offsets buffer SRV");

	SRVDesc.Buffer.NumElements = (UINT)MAX_GRASS_PER_CHUNK;

	HFALSE_IF_FAILED(Device->CreateShaderResourceView(m_OffsetsBuffer.Get(), &SRVDesc, &m_OffsetsSRV));
	NAME_D3D_RESOURCE(m_OffsetsSRV, "Frustum culler offsets buffer SRV");

	SRVDesc.Buffer.NumElements = (UINT)MAX_GRASS_COUNT;

	HFALSE_IF_FAILED(Device->CreateShaderResourceView(m_CulledGrassDataBuffer.Get(), &SRVDesc, &m_CulledGrassDataSRV));
	NAME_D3D_RESOURCE(m_CulledGrassDataSRV, "Frustum culler culled grass data buffer SRV");

	HFALSE_IF_FAILED(Device->CreateShaderResourceView(m_CulledGrassLODDataBuffer.Get(), &SRVDesc, &m_CulledGrassLODDataSRV));
	NAME_D3D_RESOURCE(m_CulledGrassLODDataSRV, "Frustum culler culled grass LOD data buffer SRV");

	uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.Buffer.NumElements = 2;

	HFALSE_IF_FAILED(Device->CreateUnorderedAccessView(m_InstanceCountBuffer.Get(), &uavDesc, &m_InstanceCountBufferUAV));
	NAME_D3D_RESOURCE(m_InstanceCountBufferUAV, "Frustum culler instance count buffer UAV");

	return true;
}

bool FrustumCuller::InitialiseStatics()
{
	HRESULT hResult;

	assert(!ms_bStaticsInitialised);
	ms_bStaticsInitialised = true;

	D3D11_BUFFER_DESC Desc = {};
	Desc.ByteWidth = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS);
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	Desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS | D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&Desc, nullptr, &ms_DummyArgsBuffer));
	NAME_D3D_RESOURCE(ms_DummyArgsBuffer, "Frustum culler dummy args buffer");

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
	uavDesc.Buffer.NumElements = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS) / 4;

	HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateUnorderedAccessView(ms_DummyArgsBuffer.Get(), &uavDesc, &ms_DummyArgsBufferUAV));
	NAME_D3D_RESOURCE(ms_DummyArgsBufferUAV, "Frustum culler dummy args buffer UAV");

	return true;
}

void FrustumCuller::UpdateBuffers(const std::vector<CullTransformData>& Transforms, const AABB& BBox, UINT* ThreadGroupCount, UINT SentInstanceCount,
	UINT GrassPerChunk, UINT PlaneDimension, float HeightDisplacement)
{
	assert(Transforms.size() <= MAX_INSTANCE_COUNT);

	HRESULT hResult;
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();
	D3D11_MAPPED_SUBRESOURCE MappedResource = {};

	ASSERT_NOT_FAILED(DeviceContext->Map(m_TransformsBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedResource));
	memcpy(MappedResource.pData, Transforms.data(), sizeof(CullTransformData) * Transforms.size());
	DeviceContext->Unmap(m_TransformsBuffer.Get(), 0u);

	UpdateCBuffer(&BBox, ThreadGroupCount, SentInstanceCount, GrassPerChunk, PlaneDimension, HeightDisplacement);
}

void FrustumCuller::UpdateBuffers(const std::vector<DirectX::XMFLOAT2>& Offsets, const AABB& BBox, UINT* ThreadGroupCount, UINT SentInstanceCount,
	UINT GrassPerChunk, UINT PlaneDimension, float HeightDisplacement)
{
	assert(Offsets.size() <= MAX_GRASS_PER_CHUNK);

	HRESULT hResult;
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();
	D3D11_MAPPED_SUBRESOURCE MappedResource = {};

	ASSERT_NOT_FAILED(DeviceContext->Map(m_OffsetsBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedResource));
	memcpy(MappedResource.pData, Offsets.data(), sizeof(DirectX::XMFLOAT2) * Offsets.size());
	DeviceContext->Unmap(m_OffsetsBuffer.Get(), 0u);

	UpdateCBuffer(&BBox, ThreadGroupCount, SentInstanceCount, GrassPerChunk, PlaneDimension, HeightDisplacement);
}

void FrustumCuller::UpdateCBuffer(const AABB* BBox, UINT* ThreadGroupCount, UINT SentInstanceCount, UINT GrassPerChunk,
	UINT PlaneDimension, float HeightDisplacement, float LODDistanceThreshold)
{
	HRESULT hResult;
	CBufferData* CBufferDataPtr;
	D3D11_MAPPED_SUBRESOURCE MappedResource = {};
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();
	
	ASSERT_NOT_FAILED(DeviceContext->Map(m_CBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedResource));
	CBufferDataPtr = (CBufferData*)MappedResource.pData;
	memcpy(CBufferDataPtr->ThreadGroupCount, ThreadGroupCount, sizeof(UINT) * 3);
	CBufferDataPtr->SentInstanceCount = SentInstanceCount;
	CBufferDataPtr->GrassPerChunk = GrassPerChunk;
	CBufferDataPtr->PlaneDimension = PlaneDimension;
	CBufferDataPtr->HeightDisplacement = HeightDisplacement;
	CBufferDataPtr->LODDistanceThreshold = LODDistanceThreshold;
	CBufferDataPtr->Min = BBox->Min;
	CBufferDataPtr->Max = BBox->Max;
	CBufferDataPtr->Padding = {};
	CBufferDataPtr->Padding1 = {};
	DeviceContext->Unmap(m_CBuffer.Get(), 0u);
}

void FrustumCuller::DispatchShaderImpl(UINT* ThreadGroupCount)
{	
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();
	const UINT InitialCount = 0u;

	DeviceContext->CSSetUnorderedAccessViews(0u, 1u, m_CulledTransformsUAV.GetAddressOf(), &InitialCount);
	DeviceContext->CSSetUnorderedAccessViews(1u, 1u, m_CulledOffsetsUAV.GetAddressOf(), &InitialCount);
	DeviceContext->CSSetUnorderedAccessViews(4u, 1u, m_InstanceCountBufferUAV.GetAddressOf(), nullptr);
	DeviceContext->CSSetShaderResources(0u, 1u, m_TransformsSRV.GetAddressOf());
	DeviceContext->CSSetShaderResources(1u, 1u, m_OffsetsSRV.GetAddressOf());
	DeviceContext->CSSetConstantBuffers(1u, 1u, m_CBuffer.GetAddressOf());
	
	DeviceContext->Dispatch(ThreadGroupCount[0], ThreadGroupCount[1], ThreadGroupCount[2]);
	m_Profiler->AddComputeDispatch();

	DeviceContext->CSSetConstantBuffers(1u, 7u, NullBuffers);
	DeviceContext->CSSetShaderResources(0u, 8u, NullSRVs);
	DeviceContext->CSSetUnorderedAccessViews(0u, 8u, NullUAVs, nullptr);
	DeviceContext->CSSetShader(nullptr, nullptr, 0u);
}

void FrustumCuller::DispatchShaderImplNew(ID3D11ShaderResourceView* TransformsSRV, ID3D11UnorderedAccessView* CulledTransformsUAV,
	UINT* ThreadGroupCount)
{
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();
	const UINT InitialCount = 0u;

	DeviceContext->CSSetUnorderedAccessViews(0u, 1u, &CulledTransformsUAV, &InitialCount);
	DeviceContext->CSSetUnorderedAccessViews(4u, 1u, m_InstanceCountBufferUAV.GetAddressOf(), nullptr);
	DeviceContext->CSSetShaderResources(0u, 1u, &TransformsSRV);
	DeviceContext->CSSetConstantBuffers(1u, 1u, m_CBuffer.GetAddressOf());

	DeviceContext->CSSetShader(m_CullingShader, nullptr, 0u);
	DeviceContext->Dispatch(ThreadGroupCount[0], ThreadGroupCount[1], ThreadGroupCount[2]);
	m_Profiler->AddComputeDispatch();

	DeviceContext->CSSetUnorderedAccessViews(0u, 1u, NullUAVs, &InitialCount);
}

void FrustumCuller::StoreInstanceCounts(std::array<UINT*, 2> OutInstanceCounts)
{
	HRESULT hResult;
	D3D11_MAPPED_SUBRESOURCE Data = {};
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();

	DeviceContext->CopyResource(m_StagingBuffer.Get(), m_InstanceCountBuffer.Get());

	std::array<UINT, 2> InstanceCounts;
	ASSERT_NOT_FAILED(DeviceContext->Map(m_StagingBuffer.Get(), 0u, D3D11_MAP_READ, 0u, &Data));
	memcpy(InstanceCounts.data(), Data.pData, sizeof(UINT) * 2);
	DeviceContext->Unmap(m_StagingBuffer.Get(), 0u);

	*OutInstanceCounts[0] = InstanceCounts[0];
	if (OutInstanceCounts[1] != nullptr)
		*OutInstanceCounts[1] = InstanceCounts[1];
}

void FrustumCuller::SendInstanceCountsNew(std::vector<ID3D11UnorderedAccessView*>* ArgsBufferUAVs)
{
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();
	DeviceContext->CSSetShader(m_InstanceCountTransferShader, nullptr, 0u);

	for (auto ArgsBuffer : *ArgsBufferUAVs)
	{
		ID3D11UnorderedAccessView* UAVs[] = { m_InstanceCountBufferUAV.Get(), ArgsBuffer, nullptr};
		DeviceContext->CSSetUnorderedAccessViews(4u, 3u, UAVs, nullptr);

		DeviceContext->Dispatch(1u, 1u, 1u);
		m_Profiler->AddComputeDispatch();

		DeviceContext->CSSetUnorderedAccessViews(4u, 2u, NullUAVs, nullptr);
	}
}
