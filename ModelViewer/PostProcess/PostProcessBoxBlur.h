#pragma once

#ifndef POSTPROCESSBOXBLUR_H
#define POSTPROCESSBOXBLUR_H

#include "PostProcess.h"

namespace PostProcessData
{
	struct BoxBlurData
	{
		DirectX::XMFLOAT2 TexelSize;
		int BlurStrength;
		float Padding;
	};
}

class PostProcessBoxBlur : public PostProcess<PostProcessData::BoxBlurData>
{
public:
	PostProcessBoxBlur(bool bActive, const PostProcessData::BoxBlurData& Data, const std::pair<int, int>& Dimensions);
	~PostProcessBoxBlur();

	void RenderControls() override;

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override;

private:
	ID3D11PixelShader* m_HorizontalPS;
	ID3D11PixelShader* m_VerticalPS;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_IntermediateRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_IntermediateSRV;

	const char* m_psFilename;
	const char* m_HorizontalEntry;
	const char* m_VerticalEntry;
};

#endif 
