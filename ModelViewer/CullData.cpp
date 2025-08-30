#define NOMINMAX

#include <cassert>

#include "CullData.h"
#include "Common.h"
#include "Graphics.h"
#include "MyMacros.h"

const UINT CullData::ms_INITIAL_BUFFER_INSTANCE_COUNT = 8u;

CullData::CullData() : m_BufferSize(ms_INITIAL_BUFFER_INSTANCE_COUNT)
{
}

void CullData::Init(AABB* BBox, UINT* OutInstanceCount, std::vector<ID3D11UnorderedAccessView*>* ArgsBufferUAVs, const std::string* Name,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* OutCulledTransformsSRV, std::vector<CullTransformData>* Transforms)
{
	m_BBox = BBox;
	m_ArgsBufferUAVs = ArgsBufferUAVs;
	m_Name = Name;
	m_Transforms = Transforms;
	m_OutInstanceCounts[0] = OutInstanceCount;
	m_OutCulledTransformsSRV = OutCulledTransformsSRV;

	const UINT SizeNeeded = std::max(m_BufferSize, (UINT)m_Transforms->size());
	while (m_BufferSize < SizeNeeded)
		m_BufferSize *= 2u;

	bool Result;
	Result = CreateBuffers();
	assert(Result);
	Result = CreateViews();
	assert(Result);
}

void CullData::InitGrass(AABB* BBox, UINT* OutInstanceCount, UINT* OutInstanceCountLOD, std::vector<ID3D11UnorderedAccessView*>* ArgsBufferUAVs,
	const std::string* Name)
{
	m_BBox = BBox;
	m_ArgsBufferUAVs = ArgsBufferUAVs;
	m_Name = Name;
	m_OutInstanceCounts[0] = OutInstanceCount;
	m_OutInstanceCounts[1] = OutInstanceCountLOD;
}

bool CullData::CreateBuffers()
{
	HRESULT hResult;
	D3D11_BUFFER_DESC Desc = {};
	ID3D11Device* Device = Graphics::GetSingletonPtr()->GetDevice();
	
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.ByteWidth = (UINT)(sizeof(CullTransformData) * m_BufferSize);
	Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	Desc.StructureByteStride = sizeof(CullTransformData);

	HFALSE_IF_FAILED(Device->CreateBuffer(&Desc, nullptr, m_TransformsBuffer.ReleaseAndGetAddressOf()));
	NAME_D3D_RESOURCE(m_TransformsBuffer, (*m_Name + " transforms buffer").c_str());

	Desc.CPUAccessFlags = 0;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

	HFALSE_IF_FAILED(Device->CreateBuffer(&Desc, nullptr, m_CulledTransformsBuffer.ReleaseAndGetAddressOf()));
	NAME_D3D_RESOURCE(m_CulledTransformsBuffer, (*m_Name + " culled transforms buffer").c_str());

	return true;
}

bool CullData::CreateViews()
{
	HRESULT hResult;
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	ID3D11Device* Device = Graphics::GetSingletonPtr()->GetDevice();

	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.NumElements = m_BufferSize;
	uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;

	HFALSE_IF_FAILED(Device->CreateUnorderedAccessView(m_CulledTransformsBuffer.Get(), &uavDesc, m_CulledTransformsUAV.ReleaseAndGetAddressOf()));
	NAME_D3D_RESOURCE(m_CulledTransformsUAV, (*m_Name + " culled transforms buffer UAV").c_str());

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.Buffer.NumElements = m_BufferSize;

	HFALSE_IF_FAILED(Device->CreateShaderResourceView(m_TransformsBuffer.Get(), &SRVDesc, m_TransformsSRV.ReleaseAndGetAddressOf()));
	NAME_D3D_RESOURCE(m_TransformsSRV, (*m_Name + " transforms buffer SRV").c_str());

	HFALSE_IF_FAILED(Device->CreateShaderResourceView(m_CulledTransformsBuffer.Get(), &SRVDesc, m_OutCulledTransformsSRV->ReleaseAndGetAddressOf()));
	NAME_D3D_RESOURCE(m_OutCulledTransformsSRV->Get(), (*m_Name + " culled transforms buffer SRV").c_str());

	return true;
}

void CullData::UpdateBuffers()
{
	assert(m_Transforms->size() <= MAX_INSTANCE_COUNT);

	if ((UINT)m_Transforms->size() > m_BufferSize)
		GrowBuffersToFit();

	HRESULT hResult;
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();
	D3D11_MAPPED_SUBRESOURCE MappedResource = {};

	ASSERT_NOT_FAILED(DeviceContext->Map(m_TransformsBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedResource));
	memcpy(MappedResource.pData, m_Transforms->data(), sizeof(CullTransformData) * m_Transforms->size());
	DeviceContext->Unmap(m_TransformsBuffer.Get(), 0u);
}

bool CullData::GrowBuffersToFit()
{
	UINT OldSize = m_BufferSize;
	while ((UINT)m_Transforms->size() > m_BufferSize)
		m_BufferSize *= 2u;
	
	if (m_BufferSize == OldSize)
		return true;

	bool Result;
	Result = CreateBuffers();
	assert(Result);
	Result = CreateViews();
	assert(Result);

	return true;
}
