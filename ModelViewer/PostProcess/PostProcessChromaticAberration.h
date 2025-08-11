#pragma once

#ifndef POSTPROCESSCHROMATICABERRATION_H
#define POSTPROCESSCHROMATICABERRATION_H

#include "PostProcess.h"

namespace PostProcessData
{
	struct ChromaticAberrationData
	{
		DirectX::XMFLOAT2 PixelSize;
		float Scale;
		float Padding;
	};
}

class PostProcessChromaticAberration : public PostProcess<PostProcessData::ChromaticAberrationData>
{
public:
	PostProcessChromaticAberration(bool bActive, PostProcessData::ChromaticAberrationData Data);
	~PostProcessChromaticAberration();

	void RenderControls() override;

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override;

private:
	const char* m_psFilename;
};

#endif
