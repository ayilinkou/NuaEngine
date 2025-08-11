#pragma once

#ifndef POSTPROCESSTEMPORALAA_H
#define POSTPROCESSTEMPORALAA_H

#include "PostProcess.h"

namespace PostProcessData
{
	struct TemporalAAData
	{
		float Alpha;
		int bUseMotionVectors;
		int bUseColorClamping;
		float Padding;
	};
}

class CameraManager;

class PostProcessTemporalAA : public PostProcess<PostProcessData::TemporalAAData>
{
public:
	PostProcessTemporalAA(bool bActive, PostProcessData::TemporalAAData Data, std::shared_ptr<CameraManager>& CamManager);
	~PostProcessTemporalAA();

	void RenderControls() override;

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override;

private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_HistoryFrameTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_HistoryFrameSRV;

	const char* m_psFilename;
};

#endif
