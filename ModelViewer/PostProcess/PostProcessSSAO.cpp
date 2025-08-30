#include <random>
#include <cmath>
#include <functional>

#include "PostProcessSSAO.h"
#include "ResourceManager.h"

PostProcessSSAO::PostProcessSSAO(bool bActive, const PostProcessData::SSAOData& Data)
	: PostProcess(bActive, Data, "SSAO"), m_psFilename("Shaders/SSAO_PS.hlsl")
{
	GenerateNoiseTexture();
	GenerateVisibilityTexture();
	SetupPixelShader(m_PixelShader, m_psFilename);

	m_OnDeactivated.Bind(std::bind(&PostProcessSSAO::ClearVisibilityTexture, this));
}

PostProcessSSAO::~PostProcessSSAO()
{
	ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, "main");
}

void PostProcessSSAO::RenderControls()
{
	bool bDirty = false;

	if (ImGui::DragFloat("Radius", &m_CurrentSettings.Radius, 0.01f, 0.f, 10.f, "%.2f"))
		bDirty = true;

	if (bDirty)
		UpdateConstantBuffer();

	IPostProcess::RenderControls();
}

void PostProcessSSAO::GenerateSamplePoints(DirectX::XMFLOAT4* SampleKernelDest)
{
	std::uniform_real_distribution<float> FloatDist(0.f, 1.f);
	std::default_random_engine Generator;
	
	for (int i = 0; i < 64; i++)
	{
		DirectX::XMVECTOR v = DirectX::XMVectorSet(
			FloatDist(Generator) * 2.f - 1.f,
			FloatDist(Generator) * 2.f - 1.f,
			FloatDist(Generator),
			0.f
		);

		v = DirectX::XMVector3Normalize(v);

		// cluster samples towards pixel origin
		float Scale = float(i) / 64.f;
		Scale = std::lerp(0.1f, 1.f, Scale * Scale);
		DirectX::XMStoreFloat4(&SampleKernelDest[i], DirectX::XMVectorScale(v, Scale));
	}
}

void PostProcessSSAO::ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV)
{
	Graphics* pGraphics = Graphics::GetSingletonPtr();
	DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);

	ID3D11ShaderResourceView* SRVs[3] = {
		pGraphics->GetDepthStencilSRV().Get(),
		pGraphics->GetNormalSRV().Get(),
		m_NoiseSRV.Get()
	};
	DeviceContext->PSSetShaderResources(0u, 3u, SRVs);
	DeviceContext->PSSetConstantBuffers(1u, 1u, m_ConstantBuffer.GetAddressOf());

	DeviceContext->OMSetRenderTargets(1u, m_VisibilityRTV.GetAddressOf(), nullptr);
	DeviceContext->DrawIndexed(6u, 0u, 0);
	GetProfiler()->AddDrawCall();
}

void PostProcessSSAO::GenerateNoiseTexture()
{
	HRESULT hResult;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> NoiseTexture;
	Graphics* pGraphics = Graphics::GetSingletonPtr();
	constexpr UINT WIDTH = 4u;
	constexpr UINT HEIGHT = 4u;

	D3D11_TEXTURE2D_DESC Desc = {};
	Desc.Format = DXGI_FORMAT_R32G32_FLOAT;
	Desc.Width = WIDTH;
	Desc.Height = HEIGHT;
	Desc.MipLevels = 1u;
	Desc.ArraySize = 1u;
	Desc.SampleDesc.Count = 1u;
	Desc.Usage = D3D11_USAGE_IMMUTABLE;
	Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	std::uniform_real_distribution<float> FloatDist(0.f, 1.f);
	std::default_random_engine Generator;

	DirectX::XMFLOAT2 NoiseData[WIDTH * HEIGHT];
	for (int i = 0; i < WIDTH * HEIGHT; i++)
	{
		NoiseData[i] = {
			FloatDist(Generator) * 2.f - 1.f,
			FloatDist(Generator) * 2.f - 1.f
		};
	}

	D3D11_SUBRESOURCE_DATA Data = {};
	Data.pSysMem = NoiseData;
	Data.SysMemPitch = WIDTH * sizeof(DirectX::XMFLOAT2);

	ASSERT_NOT_FAILED(pGraphics->GetDevice()->CreateTexture2D(&Desc, &Data, &NoiseTexture));
	ASSERT_NOT_FAILED(pGraphics->GetDevice()->CreateShaderResourceView(NoiseTexture.Get(), nullptr, &m_NoiseSRV));
}

void PostProcessSSAO::GenerateVisibilityTexture()
{
	HRESULT hResult;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> VisTexture;
	Graphics* pGraphics = Graphics::GetSingletonPtr();
	const auto& Dimensions = pGraphics->GetRenderTargetDimensions();
	const UINT WIDTH = (UINT)Dimensions.first;
	const UINT HEIGHT = (UINT)Dimensions.second;

	D3D11_TEXTURE2D_DESC Desc = {};
	Desc.Format = DXGI_FORMAT_R32_FLOAT;
	Desc.Width = WIDTH;
	Desc.Height = HEIGHT;
	Desc.MipLevels = 1u;
	Desc.ArraySize = 1u;
	Desc.SampleDesc.Count = 1u;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	ASSERT_NOT_FAILED(pGraphics->GetDevice()->CreateTexture2D(&Desc, nullptr, &VisTexture));
	ASSERT_NOT_FAILED(pGraphics->GetDevice()->CreateShaderResourceView(VisTexture.Get(), nullptr, &m_VisibilitySRV));
	ASSERT_NOT_FAILED(pGraphics->GetDevice()->CreateRenderTargetView(VisTexture.Get(), nullptr, &m_VisibilityRTV));
	ClearVisibilityTexture();
}

void PostProcessSSAO::ClearVisibilityTexture()
{
	float ClearColor[4] = { 1.f, 1.f, 1.f, 1.f };
	Graphics::GetSingletonPtr()->GetDeviceContext()->ClearRenderTargetView(m_VisibilityRTV.Get(), ClearColor);
}
