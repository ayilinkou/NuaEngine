#include "PostProcessGammaCorrection.h"
#include "ResourceManager.h"

PostProcessGammaCorrection::PostProcessGammaCorrection(bool bActive, const PostProcessData::GammaCorrectionData& Data)
	: PostProcess(bActive, Data, "Gamma Correction"), m_psFilename("Shaders/GammaCorrectionPS.hlsl")
{
	SetupPixelShader(m_PixelShader, m_psFilename);
}

PostProcessGammaCorrection::~PostProcessGammaCorrection()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename);
}

void PostProcessGammaCorrection::RenderControls()
{
	bool bDirty = false;

	if (ImGui::SliderFloat("Gamma", &m_CurrentSettings.Gamma, 1.f, 3.f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
		bDirty = true;

	if (bDirty)
		UpdateConstantBuffer();

	IPostProcess::RenderControls();
}

void PostProcessGammaCorrection::ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV)
{
	DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);
	DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());

	DeviceContext->PSSetConstantBuffers(1u, 1u, m_ConstantBuffer.GetAddressOf());

	DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);

	DeviceContext->DrawIndexed(6u, 0u, 0);
	GetProfiler()->AddDrawCall();
}