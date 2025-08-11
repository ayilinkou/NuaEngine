#include "PostProcessChromaticAberration.h"
#include "ResourceManager.h"

PostProcessChromaticAberration::PostProcessChromaticAberration(bool bActive, PostProcessData::ChromaticAberrationData Data)
	: PostProcess(bActive, Data, "Chromatic Aberration"), m_psFilename("Shaders/ChromaticAberrationPS.hlsl")
{
	SetupPixelShader(m_PixelShader, m_psFilename);
}

PostProcessChromaticAberration::~PostProcessChromaticAberration()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename);
}

void PostProcessChromaticAberration::RenderControls()
{
	bool bDirty = false;

	if (ImGui::SliderFloat("Scale", &m_CurrentSettings.Scale, -10.f, 10.f, "%.f"))
		bDirty = true;

	if (bDirty)
		UpdateConstantBuffer();

	IPostProcess::RenderControls();
}

void PostProcessChromaticAberration::ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV)
{
	DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);
	DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());

	DeviceContext->PSSetConstantBuffers(1u, 1u, m_ConstantBuffer.GetAddressOf());

	DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);

	DeviceContext->DrawIndexed(6u, 0u, 0);
	GetProfiler()->AddDrawCall();
}