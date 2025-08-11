#pragma once

#ifndef POSTPROCESSBLOOM_H
#define POSTPROCESSBLOOM_H

#include "PostProcess.h"

namespace PostProcessData
{
	struct BloomData
	{
		float LuminanceThreshold;
		DirectX::XMFLOAT3 Padding;
	};

	struct GaussianBlurData;
}

class PostProcessGaussianBlur;

class PostProcessBloom : public PostProcess<PostProcessData::BloomData>
{
public:
	PostProcessBloom(bool bActive, const PostProcessData::BloomData& Data, const PostProcessData::GaussianBlurData& GaussianData,
		const std::pair<int, int>& Dimensions);

	~PostProcessBloom();

	void RenderControls() override;

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override;

private:
	std::unique_ptr<PostProcessGaussianBlur> m_BlurPostProcess;
	const char* m_psFilename;
	const char* m_LuminanceEntry;
	const char* m_BloomEntry;

	ID3D11PixelShader* m_LuminancePS;
	ID3D11PixelShader* m_BloomPS;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_LuminousRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_BlurredRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_LuminousSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_BlurredSRV;
};

#endif
