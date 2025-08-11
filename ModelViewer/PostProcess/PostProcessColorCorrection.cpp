#include "PostProcessColorCorrection.h"
#include "ResourceManager.h"

PostProcessColorCorrection::PostProcessColorCorrection(bool bActive, const PostProcessData::ColorCorrectionData& Data)
	: PostProcess(bActive, Data, "Color Correction"), m_psFilename("Shaders/ColorCorrectionPS.hlsl")
{
	SetupPixelShader(m_PixelShader, m_psFilename);
}

PostProcessColorCorrection::~PostProcessColorCorrection()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename);
}

void PostProcessColorCorrection::RenderControls()
{
	bool bDirty = false;

	if (ImGui::SliderFloat("Contrast", &m_CurrentSettings.Contrast, 0.f, 2.f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
		bDirty = true;
	if (ImGui::SliderFloat("Brightness", &m_CurrentSettings.Brightness, -0.5f, 0.5f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
		bDirty = true;
	if (ImGui::SliderFloat("Saturation", &m_CurrentSettings.Saturation, 0.f, 5.f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
		bDirty = true;

	if (bDirty)
		UpdateConstantBuffer();

	IPostProcess::RenderControls();
}

void PostProcessColorCorrection::ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV)
{
	DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);
	DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());

	DeviceContext->PSSetConstantBuffers(1u, 1u, m_ConstantBuffer.GetAddressOf());

	DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);

	DeviceContext->DrawIndexed(6u, 0u, 0);
	GetProfiler()->AddDrawCall();
}