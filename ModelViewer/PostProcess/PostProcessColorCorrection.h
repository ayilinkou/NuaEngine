#pragma once

#ifndef POSTPROCESSCOLORCORRECTION_H
#define POSTPROCESSCOLORCORRECTION_H

#include "PostProcess.h"

namespace PostProcessData
{
	struct ColorCorrectionData
	{
		float Contrast;
		float Brightness;
		float Saturation;
		float Padding;
	};
}

class PostProcessColorCorrection : public PostProcess<PostProcessData::ColorCorrectionData>
{
public:
	PostProcessColorCorrection(bool bActive, const PostProcessData::ColorCorrectionData& Data);
	~PostProcessColorCorrection();

	void RenderControls() override;

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override;

private:
	const char* m_psFilename;
};

#endif
