#include "PostProcessToneMapper.h"
#include "ResourceManager.h"

PostProcessToneMapper::PostProcessToneMapper(bool bActive, const PostProcessData::ToneMapperData& Data)
	: PostProcess(bActive, Data, "Tone Mapper"), m_psFilename("Shaders/ToneMapperPS.hlsl")
{
	assert(static_cast<int>(Data.Formula) >= 0 && Data.Formula < PostProcessData::ToneMapperFormula::None);
	SetupPixelShader(m_PixelShader, m_psFilename);
}

PostProcessToneMapper::~PostProcessToneMapper()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename);
}

void PostProcessToneMapper::RenderControls()
{
	bool bDirty = false;
	const char* Formulas[] = { "Reinhard Basic", "Reinhard Extended", "Reinhard Extended Bias", "Narkowicz ACES", "Hill ACES" };

	if (ImGui::Combo("Formula", (int*)&m_CurrentSettings.Formula, Formulas, IM_ARRAYSIZE(Formulas)))
		bDirty = true;

	switch (m_CurrentSettings.Formula)
	{
	case PostProcessData::ToneMapperFormula::ReinhardBasic:
		break;
	case PostProcessData::ToneMapperFormula::ReinhardExtended:
		if (ImGui::SliderFloat("White Level", &m_CurrentSettings.WhiteLevel, 0.f, 10.f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;
		break;
	case PostProcessData::ToneMapperFormula::ReinhardExtendedBias:
		if (ImGui::SliderFloat("Bias", &m_CurrentSettings.Bias, 0.f, 10.f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;
		break;
	case PostProcessData::ToneMapperFormula::NarkowiczACES:
		break;
	case PostProcessData::ToneMapperFormula::HillACES:
		break;
	default:
		break;
	}

	if (bDirty)
		UpdateConstantBuffer();

	IPostProcess::RenderControls();
}

void PostProcessToneMapper::ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV)
{
	DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);
	DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());

	DeviceContext->PSSetConstantBuffers(1u, 1u, m_ConstantBuffer.GetAddressOf());

	DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);

	DeviceContext->DrawIndexed(6u, 0u, 0);
	GetProfiler()->AddDrawCall();
}