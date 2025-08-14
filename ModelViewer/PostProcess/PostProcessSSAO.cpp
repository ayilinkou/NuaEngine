#include "PostProcessSSAO.h"
#include "ResourceManager.h"

PostProcessSSAO::PostProcessSSAO(bool bActive, const PostProcessData::SSAOData& Data)
	: PostProcess(bActive, Data, "SSAO"), m_psFilename("Shaders/SSAO_PS.hlsl")
{
	SetupPixelShader(m_PixelShader, m_psFilename);
}

PostProcessSSAO::~PostProcessSSAO()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, "main");
}

void PostProcessSSAO::RenderControls()
{
	bool bDirty = false;

	if (ImGui::DragFloat("Radius", &m_CurrentSettings.Radius, 0.01f, 0.f, 1.f, "%.2f"))
		bDirty = true;

	if (bDirty)
		UpdateConstantBuffer();

	IPostProcess::RenderControls();
}

void PostProcessSSAO::ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV)
{
	Graphics* pGraphics = Graphics::GetSingletonPtr();
	DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);

	ID3D11ShaderResourceView* SRVs[3] = { SRV.Get(), pGraphics->GetDepthStencilSRV().Get(), pGraphics->GetNormalSRV().Get() };
	DeviceContext->PSSetShaderResources(0u, 3u, SRVs);
	DeviceContext->PSSetConstantBuffers(1u, 1u, m_ConstantBuffer.GetAddressOf());

	DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);
	DeviceContext->DrawIndexed(6u, 0u, 0);
	GetProfiler()->AddDrawCall();
}