#include <cassert>

#include "CullData.h"
#include "Common.h"
#include "Graphics.h"
#include "MyMacros.h"

CullData::CullData(const AABB& BBox, UINT& OutInstanceCount, const std::vector<ID3D11UnorderedAccessView*>& ArgsBufferUAVs, const std::string& Name,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& OutCulledTransformsSRV, const std::vector<CullTransformData>& Transforms) :
	m_BBox(BBox), m_OutInstanceCount(OutInstanceCount), m_ArgsBufferUAVs(ArgsBufferUAVs), m_Name(Name), m_Transforms(Transforms)
{
	bool Result;
	Result = CreateBuffers();
	assert(Result);
	Result = CreateViews(OutCulledTransformsSRV);
	assert(Result);
}

bool CullData::CreateBuffers()
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
	NAME_D3D_RESOURCE(m_TransformsBuffer, (m_Name + " transforms buffer").c_str());

	Desc.CPUAccessFlags = 0;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

	HFALSE_IF_FAILED(Device->CreateBuffer(&Desc, nullptr, &m_CulledTransformsBuffer));
	NAME_D3D_RESOURCE(m_CulledTransformsBuffer, (m_Name + " culled transforms buffer").c_str());

	return true;
}

bool CullData::CreateViews(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& OutCulledTransformsSRV)
{
	HRESULT hResult;
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	ID3D11Device* Device = Graphics::GetSingletonPtr()->GetDevice();

	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.NumElements = (UINT)MAX_INSTANCE_COUNT;
	uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;

	HFALSE_IF_FAILED(Device->CreateUnorderedAccessView(m_CulledTransformsBuffer.Get(), &uavDesc, &m_CulledTransformsUAV));
	NAME_D3D_RESOURCE(m_CulledTransformsUAV, (m_Name + " culled transforms buffer UAV").c_str());

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.Buffer.NumElements = (UINT)MAX_INSTANCE_COUNT;

	HFALSE_IF_FAILED(Device->CreateShaderResourceView(m_TransformsBuffer.Get(), &SRVDesc, &m_TransformsSRV));
	NAME_D3D_RESOURCE(m_TransformsSRV, (m_Name + " transforms buffer SRV").c_str());

	HFALSE_IF_FAILED(Device->CreateShaderResourceView(m_CulledTransformsBuffer.Get(), &SRVDesc, OutCulledTransformsSRV.GetAddressOf()));
	NAME_D3D_RESOURCE(OutCulledTransformsSRV, (m_Name + " culled transforms buffer SRV").c_str());

	return true;
}

void CullData::UpdateBuffers()
{
	assert(m_Transforms.size() <= MAX_INSTANCE_COUNT);

	HRESULT hResult;
	ID3D11DeviceContext* DeviceContext = Graphics::GetSingletonPtr()->GetDeviceContext();
	D3D11_MAPPED_SUBRESOURCE MappedResource = {};

	ASSERT_NOT_FAILED(DeviceContext->Map(m_TransformsBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedResource));
	memcpy(MappedResource.pData, m_Transforms.data(), sizeof(CullTransformData) * m_Transforms.size());
	DeviceContext->Unmap(m_TransformsBuffer.Get(), 0u);
}
