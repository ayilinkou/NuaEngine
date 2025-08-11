#include "PostProcessGaussianBlur.h"
#include "ResourceManager.h"

PostProcessGaussianBlur::PostProcessGaussianBlur(bool bActive, const PostProcessData::GaussianBlurData& Data, const std::pair<int, int>& Dimensions)
	: PostProcess(bActive, Data, "Gaussian Blur"), m_psFilename("Shaders/GaussianBlurPS.hlsl")
{
	assert(Data.BlurStrength > 0 && (UINT)Data.BlurStrength <= m_MaxBlurStrength);
	m_HorizontalEntry = "HorizontalPS";
	m_VerticalEntry = "VerticalPS";

	std::vector<float> GaussianWeights(m_MaxBlurStrength + 1, 0.f);
	FillGaussianWeights(GaussianWeights);

	D3D11_BUFFER_DESC BufferDesc = {};
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.ByteWidth = sizeof(float) * ((UINT)m_MaxBlurStrength + 1);
	BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	BufferDesc.StructureByteStride = sizeof(float);
	BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	D3D11_SUBRESOURCE_DATA BufferData = {};
	BufferData.pSysMem = GaussianWeights.data();

	HRESULT hResult;
	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &BufferData, &m_GaussianWeightsBuffer));
	NAME_D3D_RESOURCE(m_GaussianWeightsBuffer, ("Post process " + m_Name + " gaussian weights structured buffer").c_str());

	D3D11_SHADER_RESOURCE_VIEW_DESC GaussianSRVDesc = {};
	GaussianSRVDesc.Format = DXGI_FORMAT_UNKNOWN; // set to this when using a structured buffer
	GaussianSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	GaussianSRVDesc.Buffer.NumElements = m_MaxBlurStrength + 1;

	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateShaderResourceView(m_GaussianWeightsBuffer.Get(), &GaussianSRVDesc, &m_GaussianWeightsSRV));
	NAME_D3D_RESOURCE(m_GaussianWeightsSRV, ("Post process " + m_Name + " gaussian weights structured buffer SRV").c_str());

	SetupPixelShader(m_HorizontalPS, m_psFilename, m_HorizontalEntry);
	SetupPixelShader(m_VerticalPS, m_psFilename, m_VerticalEntry);

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
	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateRenderTargetView(IntermediateTexture, NULL, &m_IntermediateRTV));
	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateShaderResourceView(IntermediateTexture, NULL, &m_IntermediateSRV));

	NAME_D3D_RESOURCE(IntermediateTexture, ("Post process " + m_Name + " texture").c_str());
	NAME_D3D_RESOURCE(m_IntermediateRTV, ("Post process " + m_Name + " texture RTV").c_str());
	NAME_D3D_RESOURCE(m_IntermediateSRV, ("Post process " + m_Name + " texture SRV").c_str());

	IntermediateTexture->Release();
}

PostProcessGaussianBlur::~PostProcessGaussianBlur()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, m_HorizontalEntry);
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, m_VerticalEntry);
}

void PostProcessGaussianBlur::RenderControls()
{
	bool bDirty = true;

	if (ImGui::SliderInt("Blur Strength", &m_CurrentSettings.BlurStrength, 0, m_MaxBlurStrength, "%.d", ImGuiSliderFlags_AlwaysClamp))
		bDirty = true;
	if (ImGui::SliderFloat("Sigma", &m_CurrentSettings.Sigma, 1.f, 8.f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
		bDirty = true;

	if (bDirty)
	{
		UpdateConstantBuffer(); // TODO: these can probably be combined into a single buffer
		UpdateGaussianWeightsBuffer();
	}

	IPostProcess::RenderControls();
}

void PostProcessGaussianBlur::ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV)
{
	// horizontal
	DeviceContext->PSSetShader(m_HorizontalPS, nullptr, 0u);
	DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());
	DeviceContext->PSSetShaderResources(1u, 1u, m_GaussianWeightsSRV.GetAddressOf());

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

void PostProcessGaussianBlur::UpdateGaussianWeightsBuffer()
{
	HRESULT hResult;
	D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
	ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDeviceContext()->Map(m_GaussianWeightsBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedSubresource));
	std::vector<float> GaussianWeights(m_CurrentSettings.BlurStrength + 1, 0.f);
	FillGaussianWeights(GaussianWeights);
	memcpy(MappedSubresource.pData, GaussianWeights.data(), (m_CurrentSettings.BlurStrength + 1) * sizeof(float));
	Graphics::GetSingletonPtr()->GetDeviceContext()->Unmap(m_GaussianWeightsBuffer.Get(), 0u);
}

float PostProcessGaussianBlur::CalcGaussianWeight(int x)
{
	float AdjustedSigma = m_CurrentSettings.Sigma + 0.2f * abs(x);
	return expf(-0.5f * (x * x) / (AdjustedSigma * AdjustedSigma));
}

void PostProcessGaussianBlur::FillGaussianWeights(std::vector<float>& GaussianWeights)
{
	float Sum = 0.f;
	for (int i = 0; i <= m_CurrentSettings.BlurStrength; i++)
	{
		GaussianWeights[i] = CalcGaussianWeight(i);
		Sum += GaussianWeights[i];
	}

	// normalise the weights so that they sum to 1
	for (int i = 0; i <= m_CurrentSettings.BlurStrength; i++)
	{
		GaussianWeights[i] /= Sum;
	}
}