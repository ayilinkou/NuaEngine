#pragma once

#ifndef POSTPROCESSFOG_H
#define POSTPROCESSFOG_H

#include "PostProcess.h"

namespace PostProcessData
{
	enum class FogFormula
	{
		Linear,
		Exponential,
		ExponentialSquared,
		None
	};

	struct FogData
	{
		DirectX::XMFLOAT3 FogColor;
		FogFormula Formula;
		float Density;
		DirectX::XMFLOAT3 Padding;
	};
}

class PostProcessFog : public PostProcess<PostProcessData::FogData>
{
public:
	PostProcessFog(bool bActive, const PostProcessData::FogData& Data);
	~PostProcessFog();

	virtual void RenderControls() override;

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override;

private:
	const char* m_psFilename;
};

#endif
