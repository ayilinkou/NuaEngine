#pragma once

#ifndef POSTPROCESSGAUSSIANBLUR_H
#define POSTPROCESSGAUSSIANBLUR_H

#include "PostProcess.h"

namespace PostProcessData
{
	struct GaussianBlurData
	{
		DirectX::XMFLOAT2 TexelSize;
		int BlurStrength;
		float Sigma;
	};
}

class PostProcessGaussianBlur : public PostProcess<PostProcessData::GaussianBlurData>
{
public:
	PostProcessGaussianBlur(bool bActive, const PostProcessData::GaussianBlurData& Data, const std::pair<int, int>& Dimensions);
	~PostProcessGaussianBlur();

	void RenderControls() override;

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override;

	void UpdateGaussianWeightsBuffer();
	float CalcGaussianWeight(int x);
	void FillGaussianWeights(std::vector<float>& GaussianWeights);

private:
	const UINT m_MaxBlurStrength = 100;
	const char* m_psFilename;
	const char* m_HorizontalEntry;
	const char* m_VerticalEntry;

	ID3D11PixelShader* m_HorizontalPS;
	ID3D11PixelShader* m_VerticalPS;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_GaussianWeightsBuffer;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_IntermediateRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_IntermediateSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_GaussianWeightsSRV;
};

#endif
