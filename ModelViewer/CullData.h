#pragma once

#ifndef CULLDATA_H
#define CULLDATA_H

#include <vector>
#include <string>

#include "d3d11.h"

#include "wrl.h"

struct AABB;
struct CullTransformData;

class CullData
{
public:	
	CullData() = delete;
	CullData(const AABB& BBox, UINT& OutInstanceCount, const std::vector<ID3D11UnorderedAccessView*>& ArgsBufferUAVs, const std::string& Name,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& OutCulledTransformsSRV, const std::vector<CullTransformData>& Transforms);

	bool CreateBuffers();
	bool CreateViews(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& OutCulledTransformsSRV);
	void UpdateBuffers();

	UINT GetSentInstanceCount() const { return (UINT)m_Transforms.size(); }
	UINT& GetOutInstanceCount() { return m_OutInstanceCount; }
	const AABB& GetBoundingBox() const { return m_BBox; }
	ID3D11ShaderResourceView* GetTransformsSRV() { return m_TransformsSRV.Get(); }
	ID3D11UnorderedAccessView* GetCulledTransformsUAV() { return m_CulledTransformsUAV.Get(); }
	const std::vector<ID3D11UnorderedAccessView*>& GetArgsBufferUAVs() { return m_ArgsBufferUAVs; }

private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_TransformsSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_CulledTransformsUAV;
	UINT& m_OutInstanceCount;
	const AABB& m_BBox;
	const std::vector<ID3D11UnorderedAccessView*>& m_ArgsBufferUAVs;
	const std::vector<CullTransformData>& m_Transforms;

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_TransformsBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_CulledTransformsBuffer;

	std::string m_Name;
};

#endif
