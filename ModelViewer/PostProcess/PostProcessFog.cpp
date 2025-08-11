#include "PostProcessFog.h"
#include "ResourceManager.h"

PostProcessFog::PostProcessFog(bool bActive, const PostProcessData::FogData& Data)
	: PostProcess(bActive, Data, "Fog"), m_psFilename("Shaders/FogPS.hlsl")
{
	assert(static_cast<int>(Data.Formula) >= 0 && Data.Formula < PostProcessData::FogFormula::None);
	SetupPixelShader(m_PixelShader, m_psFilename);
}

PostProcessFog::~PostProcessFog()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, "main");
}

void PostProcessFog::RenderControls()
{
	bool bDirty = false;

	if (ImGui::ColorEdit3("Fog Color", reinterpret_cast<float*>(&m_CurrentSettings.FogColor)))
		bDirty = true;

	const char* Formulas[] = { "Linear", "Exponential", "Exponential Squared" };
	if (ImGui::Combo("Formula", reinterpret_cast<int*>(&m_CurrentSettings.Formula), Formulas, IM_ARRAYSIZE(Formulas)))
		bDirty = true;

	switch (m_CurrentSettings.Formula)
	{
	case PostProcessData::FogFormula::Linear:
		break;
	case PostProcessData::FogFormula::Exponential:
		if (ImGui::SliderFloat("Density", &m_CurrentSettings.Density, 0.f, 0.02f, "%.4f", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;
		break;
	case PostProcessData::FogFormula::ExponentialSquared:
		if (ImGui::SliderFloat("Density", &m_CurrentSettings.Density, 0.f, 0.02f, "%.4f", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;
		break;
	default:
		break;
	}

	if (bDirty)
		UpdateConstantBuffer();

	IPostProcess::RenderControls();
}

void PostProcessFog::ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV)
{
	DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);

	ID3D11ShaderResourceView* SRVs[2] = { SRV.Get(), Graphics::GetSingletonPtr()->GetDepthStencilSRV().Get() };
	DeviceContext->PSSetShaderResources(0u, 2u, SRVs);
	DeviceContext->PSSetConstantBuffers(1u, 1u, m_ConstantBuffer.GetAddressOf());

	DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);
	DeviceContext->DrawIndexed(6u, 0u, 0);
	GetProfiler()->AddDrawCall();
}