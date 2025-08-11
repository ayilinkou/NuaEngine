#pragma once

#ifndef POSTPROCESSEMPTY_H
#define POSTPROCESSEMPTY_H

#include "PostProcess.h"
#include "ResourceManager.h"

class PostProcessEmpty : public PostProcess<BYTE> // the templated settings is not used, just assigning a single byte to please compiler
{
public:
	PostProcessEmpty() : PostProcess(true, '0', "Empty", false), m_psFilename("Shaders/QuadPS.hlsl")
	{
		SetupPixelShader(m_PixelShader, m_psFilename);
	}

	virtual void RenderControls() override { IPostProcess::RenderControls(); }

	~PostProcessEmpty()
	{
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, "main");
	}

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override
	{
		DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);
		DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());

		DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);

		DeviceContext->DrawIndexed(6u, 0u, 0);
		GetProfiler()->AddDrawCall();
	}

private:
	const char* m_psFilename;
};

#endif
