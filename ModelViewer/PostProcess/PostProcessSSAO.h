#pragma once

#ifndef POSTPROCESSSSAO_H
#define POSTPROCESSSSAO_H

#include "PostProcess.h"

namespace PostProcessData
{
	struct SSAOData
	{
		float Radius;
		DirectX::XMFLOAT3 Padding;
		DirectX::XMFLOAT4 SampleKernel[64];
	};
}

class PostProcessSSAO : public PostProcess<PostProcessData::SSAOData>
{
public:
	PostProcessSSAO(bool bActive, const PostProcessData::SSAOData& Data);
	~PostProcessSSAO();

	virtual void RenderControls() override;
	
	static void GenerateSamplePoints(DirectX::XMFLOAT4* SampleKernelDest);

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override;

	void GenerateNoiseTexture();

private:
	const char* m_psFilename;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_NoiseSRV;
};

#endif
