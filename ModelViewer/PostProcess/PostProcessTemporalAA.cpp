#include "PostProcessTemporalAA.h"
#include "PostProcessEmpty.h"
#include "CameraManager.h"
#include "ResourceManager.h"

PostProcessTemporalAA::PostProcessTemporalAA(bool bActive, PostProcessData::TemporalAAData Data, std::shared_ptr<CameraManager>& CamManager)
	: PostProcess(bActive, Data, "Temporal AA"), m_psFilename("Shaders/TemporalAA_PS.hlsl")
{
	HRESULT hResult;
	Microsoft::WRL::ComPtr<ID3D11Resource> TexResource;
	Graphics::GetSingletonPtr()->m_PostProcessSRVFirst->GetResource(&TexResource);
	assert(TexResource.Get());

	Microsoft::WRL::ComPtr<ID3D11Texture2D> PostProcessTexture;
	ASSERT_NOT_FAILED(TexResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&PostProcessTexture));

	D3D11_TEXTURE2D_DESC TextureDesc;
	PostProcessTexture->GetDesc(&TextureDesc);

	ID3D11Device* pDevice = Graphics::GetSingletonPtr()->GetDevice();
	ASSERT_NOT_FAILED(pDevice->CreateTexture2D(&TextureDesc, nullptr, &m_HistoryFrameTexture));
	ASSERT_NOT_FAILED(pDevice->CreateShaderResourceView(m_HistoryFrameTexture.Get(), nullptr, &m_HistoryFrameSRV));

	NAME_D3D_RESOURCE(m_HistoryFrameTexture, "TAA history frame texture");
	NAME_D3D_RESOURCE(m_HistoryFrameSRV, "TAA history frame SRV");

	SetupPixelShader(m_PixelShader, m_psFilename);

	CamManager->BindOnActiveCameraChanged([this]() { this->m_bRecentlyActivated = true; });
}

PostProcessTemporalAA::~PostProcessTemporalAA()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename);
}

void PostProcessTemporalAA::RenderControls()
{
	bool bDirty = false;

	if (ImGui::SliderFloat("Alpha", &m_CurrentSettings.Alpha, 0.f, 1.f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
		bDirty = true;
	if (ImGui::Checkbox("Use Motion Vectors", (bool*)&m_CurrentSettings.bUseMotionVectors))
		bDirty = true;
	if (ImGui::Checkbox("Use Color Clamping", (bool*)&m_CurrentSettings.bUseColorClamping))
		bDirty = true;

	if (bDirty)
		UpdateConstantBuffer();

	IPostProcess::RenderControls();
}

void PostProcessTemporalAA::ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV)
{
	if (m_bRecentlyActivated)
	{
		// reset the history frame texture since it's old/empty
		// ideally we'd remove this draw call but since it would be just for a single frame it's not that important

		DeviceContext->PSSetShader(PostProcess::GetEmptyPostProcess()->GetPixelShader().Get(), nullptr, 0u);
		DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());

		DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);

		DeviceContext->DrawIndexed(6u, 0u, 0);
		GetProfiler()->AddDrawCall();
		m_bRecentlyActivated = false;
	}
	else
	{
		DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);
		ID3D11ShaderResourceView* SRVs[3] = { SRV.Get(), m_HistoryFrameSRV.Get(), Graphics::GetSingletonPtr()->GetVelocitySRV().Get() };
		DeviceContext->PSSetShaderResources(0u, 3u, SRVs);

		DeviceContext->PSSetConstantBuffers(1u, 1u, m_ConstantBuffer.GetAddressOf());

		DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);

		DeviceContext->DrawIndexed(6u, 0u, 0);
		GetProfiler()->AddDrawCall();
	}

	// copy RTV's texture data into m_HistoryFrameTexture
	Microsoft::WRL::ComPtr<ID3D11Resource> TexResource;
	RTV->GetResource(&TexResource);
	assert(TexResource.Get());

	DeviceContext->CopyResource(m_HistoryFrameTexture.Get(), TexResource.Get());
}