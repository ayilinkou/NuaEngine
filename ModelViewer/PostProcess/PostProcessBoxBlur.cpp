#include "PostProcessBoxBlur.h"
#include "ResourceManager.h"

PostProcessBoxBlur::PostProcessBoxBlur(bool bActive, const PostProcessData::BoxBlurData& Data, const std::pair<int, int>& Dimensions)
	: PostProcess(bActive, Data, "Box Blur"), m_psFilename("Shaders/BoxBlurPS.hlsl")
{
	assert(Data.BlurStrength > 0);
	m_HorizontalEntry = "HorizontalPS";
	m_VerticalEntry = "VerticalPS";

	SetupPixelShader(m_HorizontalPS, m_psFilename, m_HorizontalEntry);
	SetupPixelShader(m_VerticalPS, m_psFilename, m_VerticalEntry);

	HRESULT hResult;
	D3D11_TEXTURE2D_DESC TextureDesc = {};
	TextureDesc.Width = Dimensions.first;
	TextureDesc.Height = Dimensions.second;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	ID3D11Texture2D* IntermediateTexture;
	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateTexture2D(&TextureDesc, nullptr, &IntermediateTexture));
	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateRenderTargetView(IntermediateTexture, nullptr, &m_IntermediateRTV));
	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateShaderResourceView(IntermediateTexture, nullptr, &m_IntermediateSRV));

	NAME_D3D_RESOURCE(IntermediateTexture, ("Post process " + m_Name + " texture").c_str());
	NAME_D3D_RESOURCE(m_IntermediateRTV, ("Post process " + m_Name + " texture RTV").c_str());
	NAME_D3D_RESOURCE(m_IntermediateSRV, ("Post process " + m_Name + " texture SRV").c_str());

	IntermediateTexture->Release();
}

PostProcessBoxBlur::~PostProcessBoxBlur()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, m_HorizontalEntry);
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, m_VerticalEntry);
}

void PostProcessBoxBlur::RenderControls()
{
	bool bDirty = false;

	ImGui::Text("Blur Strength is currently hard coded in the shader. Fix it eventually."); // TODO
	if (ImGui::SliderInt("Blur Strength", &m_CurrentSettings.BlurStrength, 1, 32, "%.d", ImGuiSliderFlags_AlwaysClamp))
		bDirty = true;

	if (bDirty)
		UpdateConstantBuffer();

	IPostProcess::RenderControls();
}

void PostProcessBoxBlur::ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV)
{
	// horizontal
	DeviceContext->PSSetShader(m_HorizontalPS, nullptr, 0u);
	DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());

	DeviceContext->PSSetConstantBuffers(1u, 1u, m_ConstantBuffer.GetAddressOf());

	DeviceContext->OMSetRenderTargets(1u, m_IntermediateRTV.GetAddressOf(), nullptr);

	DeviceContext->DrawIndexed(6u, 0u, 0);
	GetProfiler()->AddDrawCall();

	// vertical
	ID3D11RenderTargetView* NullRTVs[] = { nullptr };
	DeviceContext->OMSetRenderTargets(1u, NullRTVs, nullptr);

	DeviceContext->PSSetShader(m_VerticalPS, nullptr, 0u);
	DeviceContext->PSSetShaderResources(0u, 1u, m_IntermediateSRV.GetAddressOf());

	DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);

	DeviceContext->DrawIndexed(6u, 0u, 0);
	GetProfiler()->AddDrawCall();
}