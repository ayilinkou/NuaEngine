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
	};
}

class PostProcessSSAO : public PostProcess<PostProcessData::SSAOData>
{
public:
	PostProcessSSAO(bool bActive, const PostProcessData::SSAOData& Data);
	~PostProcessSSAO();

	virtual void RenderControls() override;

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override;

private:
	const char* m_psFilename;
};

#endif
