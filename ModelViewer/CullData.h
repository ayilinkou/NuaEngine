#pragma once

#ifndef CULLDATA_H
#define CULLDATA_H

#include <vector>
#include <string>
#include <array>

#include "d3d11.h"
#include "DirectXMath.h"

#include "wrl.h"

struct CullTransformData
{
	DirectX::XMMATRIX CurrTransform;
	DirectX::XMMATRIX PrevTransform;
};

struct AABB;
struct CullTransformData;

class CullData
{
public:	
	CullData() {}
	void Init(AABB* BBox, UINT* OutInstanceCount, std::vector<ID3D11UnorderedAccessView*>* ArgsBufferUAVs, const std::string* Name,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* OutCulledTransformsSRV, std::vector<CullTransformData>* Transforms);
	void InitGrass(AABB* BBox, UINT* OutInstanceCount, UINT* OutInstanceCountLOD, std::vector<ID3D11UnorderedAccessView*>* ArgsBufferUAVs,
		const std::string* Name);

	bool CreateBuffers();
	bool CreateViews(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* OutCulledTransformsSRV);
	void UpdateBuffers();

	UINT GetSentInstanceCount() const { return (UINT)m_Transforms->size(); }
	std::array<UINT*, 2> GetOutInstanceCounts() { return m_OutInstanceCounts; }
	AABB* GetBoundingBox() const { return m_BBox; }
	ID3D11ShaderResourceView* GetTransformsSRV() { return m_TransformsSRV.Get(); }
	ID3D11UnorderedAccessView* GetCulledTransformsUAV() { return m_CulledTransformsUAV.Get(); }
	std::vector<ID3D11UnorderedAccessView*>* GetArgsBufferUAVs() { return m_ArgsBufferUAVs; }

private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_TransformsSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_CulledTransformsUAV;
	std::array<UINT*, 2> m_OutInstanceCounts { nullptr, nullptr };
	AABB* m_BBox;
	std::vector<ID3D11UnorderedAccessView*>* m_ArgsBufferUAVs;
	std::vector<CullTransformData>* m_Transforms;

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_TransformsBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_CulledTransformsBuffer;

	const std::string* m_Name;
};

#endif
