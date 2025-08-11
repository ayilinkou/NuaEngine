#pragma once

#ifndef POSTPROCESSTONEMAPPER_H
#define POSTPROCESSTONEMAPPER_H

#include "PostProcess.h"

namespace PostProcessData
{
	enum class ToneMapperFormula
	{
		ReinhardBasic,
		ReinhardExtended,
		ReinhardExtendedBias,
		NarkowiczACES,
		HillACES,
		None
	};

	struct ToneMapperData
	{
		float WhiteLevel;
		float Exposure;
		float Bias;
		ToneMapperFormula Formula;
	};
}

class PostProcessToneMapper : public PostProcess<PostProcessData::ToneMapperData>
{
public:
	PostProcessToneMapper(bool bActive, const PostProcessData::ToneMapperData& Data);
	~PostProcessToneMapper();

	void RenderControls() override;

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override;

private:
	const char* m_psFilename;
};

#endif
