#pragma once

#ifndef POSTPROCESSGAMMACORRECTION_H
#define POSTPROCESSGAMMACORRECTION_H

#include "PostProcess.h"

namespace PostProcessData
{
	struct GammaCorrectionData
	{
		float Gamma;
		DirectX::XMFLOAT3 Padding = {};
	};
}

class PostProcessGammaCorrection : public PostProcess<PostProcessData::GammaCorrectionData>
{
public:
	PostProcessGammaCorrection(bool bActive, const PostProcessData::GammaCorrectionData& Data);
	~PostProcessGammaCorrection();

	void RenderControls() override;

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override;

private:
	const char* m_psFilename;
};

#endif
