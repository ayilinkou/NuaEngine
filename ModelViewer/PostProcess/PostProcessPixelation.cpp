#include "PostProcessPixelation.h"
#include "ResourceManager.h"

PostProcessPixelation::PostProcessPixelation(bool bActive, const PostProcessData::PixelationData& Data)
	: PostProcess(bActive, Data, "Pixelation"), m_psFilename("Shaders/PixelationPS.hlsl")
{
	SetupPixelShader(m_PixelShader, m_psFilename);
}

PostProcessPixelation::~PostProcessPixelation()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename);
}

void PostProcessPixelation::RenderControls()
{
	bool bDirty = false;

	if (ImGui::SliderFloat("Pixel Size", &m_CurrentSettings.PixelSize, 1.f, 32.f, "%.0f", ImGuiSliderFlags_AlwaysClamp))
		bDirty = true;

	if (bDirty)
		UpdateConstantBuffer();

	IPostProcess::RenderControls();
}

void PostProcessPixelation::ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV)
{
	DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);
	DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());
	DeviceContext->PSSetConstantBuffers(1u, 1u, m_ConstantBuffer.GetAddressOf());

	DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);
	DeviceContext->DrawIndexed(6u, 0u, 0);
	GetProfiler()->AddDrawCall();
}