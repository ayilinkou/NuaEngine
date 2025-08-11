#pragma once

#ifndef POSTPROCESSPIXELATION_H
#define POSTPROCESSPIXELATION_H

#include "PostProcess.h"

namespace PostProcessData
{
	struct PixelationData
	{
		DirectX::XMFLOAT2 TruePixelSize;
		float PixelSize;
		float Padding;
	};
}

class PostProcessPixelation : public PostProcess<PostProcessData::PixelationData>
{
public:
	PostProcessPixelation(bool bActive, const PostProcessData::PixelationData& Data);
	~PostProcessPixelation();

	void RenderControls() override;

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override;

private:
	const char* m_psFilename;
};

#endif
