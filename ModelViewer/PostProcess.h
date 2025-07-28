#pragma once

#ifndef POSTPROCESS_H
#define POSTPROCESS_H

#include <iostream>
#include <cassert>

#include "ImGui/imgui.h"

#include "MyMacros.h"
#include "Graphics.h"
#include "Camera.h"
#include "ResourceManager.h"
#include "Common.h"
#include "CameraManager.h"
#include "Profiler.h"
#include "Delegate.h"

class PostProcessEmpty;

class IPostProcess
{
public:
	virtual ~IPostProcess() {}
	virtual void ResetToDefaults() = 0;

	virtual void RenderControls()
	{
		if (m_pOwner == nullptr && ImGui::Button("Reset to defaults"))
		{
			ResetToDefaults();
			for (IPostProcess* pPostProcess : m_OwnedPostProcesses)
			{
				pPostProcess->ResetToDefaults();
			}
		}
	}

public:
	void ApplyPostProcess(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV)
	{
		Graphics::GetSingletonPtr()->GetDeviceContext()->OMSetRenderTargets(8u, NullRTVs, nullptr);
		Graphics::GetSingletonPtr()->GetDeviceContext()->PSSetShaderResources(0u, 8u, NullSRVs);
		ApplyPostProcessImpl(DeviceContext, RTV, SRV);
	}

	static void InitStatics(std::shared_ptr<Profiler> Profiler)
	{
		HRESULT hResult;
		Microsoft::WRL::ComPtr<ID3D10Blob> vsBuffer;
		D3D11_INPUT_ELEMENT_DESC VertexLayout[3] = {};
		D3D11_DEPTH_STENCIL_DESC DepthStencilDesc = {};
		unsigned int NumElements;
		GetProfiler() = Profiler;

		ms_QuadVertexShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11VertexShader>(ms_vsFilename, "main", vsBuffer);

		VertexLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		VertexLayout[0].SemanticName = "POSITION";
		VertexLayout[0].SemanticIndex = 0;
		VertexLayout[0].InputSlot = 0;
		VertexLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		VertexLayout[0].AlignedByteOffset = 0;
		VertexLayout[0].InstanceDataStepRate = 0;

		VertexLayout[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		VertexLayout[1].SemanticName = "NORMAL";
		VertexLayout[1].SemanticIndex = 0;
		VertexLayout[1].InputSlot = 0;
		VertexLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		VertexLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		VertexLayout[1].InstanceDataStepRate = 0;

		VertexLayout[2].Format = DXGI_FORMAT_R32G32_FLOAT;
		VertexLayout[2].SemanticName = "TEXCOORD";
		VertexLayout[2].SemanticIndex = 0;
		VertexLayout[2].InputSlot = 0;
		VertexLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		VertexLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		VertexLayout[2].InstanceDataStepRate = 0;

		NumElements = sizeof(VertexLayout) / sizeof(VertexLayout[0]);
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateInputLayout(VertexLayout, NumElements, vsBuffer->GetBufferPointer(), vsBuffer->GetBufferSize(), &ms_QuadInputLayout));
		NAME_D3D_RESOURCE(ms_QuadInputLayout, "Post process quad input layout");

		Vertex QuadVertices[] = {
			{ DirectX::XMFLOAT3(-1.0f,  1.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 0.0f), },
			{ DirectX::XMFLOAT3(1.0f,  1.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 0.0f), },
			{ DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 1.0f), },
			{ DirectX::XMFLOAT3(1.0f, -1.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 1.0f), },
		};

		D3D11_BUFFER_DESC BufferDesc = {};
		BufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		BufferDesc.ByteWidth = sizeof(QuadVertices);
		BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		BufferDesc.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA InitData = {};
		InitData.pSysMem = QuadVertices;

		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &InitData, &ms_QuadVertexBuffer));
		NAME_D3D_RESOURCE(ms_QuadVertexBuffer, "Post process quad vertex buffer");

		unsigned int QuadIndices[] = {
			1, 2, 0,
			3, 2, 1
		};

		BufferDesc = {};
		BufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		BufferDesc.ByteWidth = sizeof(QuadIndices);
		BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		BufferDesc.CPUAccessFlags = 0;

		InitData = {};
		InitData.pSysMem = QuadIndices;

		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &InitData, &ms_QuadIndexBuffer));
		NAME_D3D_RESOURCE(ms_QuadIndexBuffer, "Post process quad index buffer");

		ms_bInitialised = true;
	}

	static void ShutdownStatics()
	{
		ms_QuadInputLayout.Reset();
		ms_QuadVertexBuffer.Reset();
		ms_QuadIndexBuffer.Reset();
		ms_EmptyPostProcess.reset();
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11VertexShader>(ms_vsFilename, "main");

		ms_bInitialised = false;
	}

	static ID3D11VertexShader* GetQuadVertexShader()
	{
		return IPostProcess::ms_QuadVertexShader;
	}

	static Microsoft::WRL::ComPtr<ID3D11Buffer> GetQuadVertexBuffer()
	{
		return IPostProcess::ms_QuadVertexBuffer;
	}

	static Microsoft::WRL::ComPtr<ID3D11Buffer> GetQuadIndexBuffer()
	{
		return IPostProcess::ms_QuadIndexBuffer;
	}

	static Microsoft::WRL::ComPtr<ID3D11InputLayout> GetQuadInputLayout()
	{
		return IPostProcess::ms_QuadInputLayout;
	}

	static std::shared_ptr<Profiler>& GetProfiler()
	{
		return IPostProcess::ms_Profiler;
	}

	static std::shared_ptr<PostProcessEmpty> GetEmptyPostProcess()
	{
		if (ms_EmptyPostProcess.get())
		{
			return IPostProcess::ms_EmptyPostProcess;
		}

		ms_EmptyPostProcess = std::make_shared<PostProcessEmpty>();
		return IPostProcess::ms_EmptyPostProcess;
	}

	void ResetToInitialActive()
	{
		SetActive(m_bInitialActive);
	}

	Microsoft::WRL::ComPtr<ID3D11PixelShader> GetPixelShader() const { return m_PixelShader; };

	void SetActive(bool bNewActive) { bNewActive ? Activate() : Deactivate(); }
	void Activate() { m_bActive = true; m_bRecentlyActivated = true; }
	void Deactivate() { m_bActive = false; }
	void SetOwner(IPostProcess* pOwner) { m_pOwner = pOwner; }
	bool IsActive() const { return m_bActive; }
	bool& GetIsActive() { return m_bActive; }
	const std::string& GetName() const { return m_Name; }

protected:
	IPostProcess(bool bStartActive, const std::string& Name) : m_bInitialActive(bStartActive), m_Name(Name) { SetActive(bStartActive); }
	
	virtual void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) = 0;

	bool SetupPixelShader(ID3D11PixelShader*& PixelShader, const char* PSFilepath, const char* EntryFunc = "main")
	{
		PixelShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11PixelShader>(PSFilepath, EntryFunc);
		assert(PixelShader);

		return true;
	}

protected:
	struct Vertex
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT2 TexCoord;
	};

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ConstantBuffer;
	ID3D11PixelShader* m_PixelShader = nullptr;
	bool m_bActive = true;
	bool m_bInitialActive = true;
	bool m_bRecentlyActivated = true;
	std::string m_Name = "";
	IPostProcess* m_pOwner = nullptr;
	std::vector<IPostProcess*> m_OwnedPostProcesses;

private:
	static ID3D11VertexShader* ms_QuadVertexShader;
	static Microsoft::WRL::ComPtr<ID3D11InputLayout> ms_QuadInputLayout;
	static Microsoft::WRL::ComPtr<ID3D11Buffer> ms_QuadVertexBuffer;
	static Microsoft::WRL::ComPtr<ID3D11Buffer> ms_QuadIndexBuffer;
	static std::shared_ptr<PostProcessEmpty> ms_EmptyPostProcess;
	static std::shared_ptr<Profiler> ms_Profiler;
	static const char* ms_vsFilename;
	static bool ms_bInitialised;
};

template <typename PostProcessDataType>
class PostProcess : public IPostProcess
{
public:
	virtual void ResetToDefaults() override
	{
		memcpy(&m_CurrentSettings, &m_InitialSettings, sizeof(PostProcessDataType));
		UpdateConstantBuffer();
		m_OnResetToDefaults.Broadcast();
	}

protected:
	PostProcess(bool bStartActive, const PostProcessDataType& Data, const std::string& Name, bool bAutoSetupCBuffer = true)
		: IPostProcess(bStartActive, Name)
	{
		m_InitialSettings = Data;
		m_CurrentSettings = Data;

		if (bAutoSetupCBuffer)
		{
			HRESULT hResult;
			ASSERT_NOT_FAILED(SetupConstantBuffer());
		}
	}

	bool SetupConstantBuffer()
	{
		HRESULT hResult;
		D3D11_BUFFER_DESC BufferDesc = {};
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.ByteWidth = sizeof(PostProcessDataType);
		BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		D3D11_SUBRESOURCE_DATA BufferData = {};
		BufferData.pSysMem = &m_InitialSettings;

		HFALSE_IF_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &BufferData, &m_ConstantBuffer));
		NAME_D3D_RESOURCE(m_ConstantBuffer, ("Post process " + m_Name + " constant buffer").c_str());

		return true;
	}

	void UpdateConstantBuffer()
	{
		HRESULT hResult;
		D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDeviceContext()->Map(m_ConstantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedSubresource));
		memcpy(MappedSubresource.pData, &m_CurrentSettings, sizeof(PostProcessDataType));
		Graphics::GetSingletonPtr()->GetDeviceContext()->Unmap(m_ConstantBuffer.Get(), 0u);
	}

protected:
	PostProcessDataType m_InitialSettings;
	PostProcessDataType m_CurrentSettings;

	Delegate<void()> m_OnResetToDefaults;
};

/////////////////////////////////////////////////////////////////////////////////

class PostProcessEmpty : public PostProcess<BYTE> // the templated settings is not used, just assigning a single byte to please compiler
{
public:
	PostProcessEmpty() : PostProcess(true, '0', "Empty", false), m_psFilename("Shaders/QuadPS.hlsl")
	{
		SetupPixelShader(m_PixelShader, m_psFilename);
	}

	virtual void RenderControls() override { IPostProcess::RenderControls(); }

	~PostProcessEmpty()
	{
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, "main");
	}

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override
	{
		DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);
		DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());

		DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);

		DeviceContext->DrawIndexed(6u, 0u, 0);
		GetProfiler()->AddDrawCall();
	}

private:
	const char* m_psFilename;
};

/////////////////////////////////////////////////////////////////////////////////

namespace PostProcessData
{
	enum class FogFormula
	{
		Linear,
		Exponential,
		ExponentialSquared,
		None
	};
	
	struct FogData
	{		
		DirectX::XMFLOAT3 FogColor;
		FogFormula Formula;
		float Density;
		float NearPlane;
		float FarPlane;
		float Padding;
	};
}

class PostProcessFog : public PostProcess<PostProcessData::FogData>
{
using enum PostProcessData::FogFormula;

public:
	PostProcessFog(bool bActive, const PostProcessData::FogData& Data) : PostProcess(bActive, Data, "Fog"), m_psFilename("Shaders/FogPS.hlsl")
	{
		assert(static_cast<int>(Data.Formula) >= 0 && Data.Formula < None);
		SetupPixelShader(m_PixelShader, m_psFilename);
	}

	~PostProcessFog()
	{
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, "main");
	}

	virtual void RenderControls()
	{
		bool bDirty = false;
		
		if (ImGui::ColorEdit3("Fog Color", reinterpret_cast<float*>(&m_CurrentSettings.FogColor)))
			bDirty = true;

		const char* Formulas[] = { "Linear", "Exponential", "Exponential Squared" };
		if (ImGui::Combo("Formula", reinterpret_cast<int*>(&m_CurrentSettings.Formula), Formulas, IM_ARRAYSIZE(Formulas)))
			bDirty = true;

		switch (m_CurrentSettings.Formula)
		{
		case Linear:
			break;
		case Exponential:
			if (ImGui::SliderFloat("Density", &m_CurrentSettings.Density, 0.f, 0.02f, "%.4f", ImGuiSliderFlags_AlwaysClamp))
				bDirty = true;
			break;
		case ExponentialSquared:
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

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override
	{
		DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);

		ID3D11ShaderResourceView* SRVs[2] = { SRV.Get(), Graphics::GetSingletonPtr()->GetDepthStencilSRV().Get() };
		DeviceContext->PSSetShaderResources(0u, 2u, SRVs);
		DeviceContext->PSSetConstantBuffers(0u, 1u, m_ConstantBuffer.GetAddressOf());

		DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);
		DeviceContext->DrawIndexed(6u, 0u, 0);
		GetProfiler()->AddDrawCall();
	}

private:
	const char* m_psFilename;
};

/////////////////////////////////////////////////////////////////////////////////

namespace PostProcessData
{
	struct BoxBlurData
	{
		DirectX::XMFLOAT2 TexelSize;
		int BlurStrength;
		float Padding;
	};
}

class PostProcessBoxBlur : public PostProcess<PostProcessData::BoxBlurData>
{
public:
	PostProcessBoxBlur(bool bActive, const PostProcessData::BoxBlurData& Data, const std::pair<int, int>& Dimensions)
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

	~PostProcessBoxBlur()
	{
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, m_HorizontalEntry);
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, m_VerticalEntry);
	}

	void RenderControls() override
	{
		bool bDirty = false;
		
		ImGui::Text("Blur Strength is currently hard coded in the shader. Fix it eventually."); // TODO
		if (ImGui::SliderInt("Blur Strength", &m_CurrentSettings.BlurStrength, 1, 32, "%.d", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;

		if (bDirty)
			UpdateConstantBuffer();

		IPostProcess::RenderControls();
	}

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override
	{
		// horizontal
		DeviceContext->PSSetShader(m_HorizontalPS, nullptr, 0u);
		DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());

		DeviceContext->PSSetConstantBuffers(0u, 1u, m_ConstantBuffer.GetAddressOf());

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

private:
	ID3D11PixelShader* m_HorizontalPS;
	ID3D11PixelShader* m_VerticalPS;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_IntermediateRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_IntermediateSRV;

	const char* m_psFilename;
	const char* m_HorizontalEntry;
	const char* m_VerticalEntry;
};

/////////////////////////////////////////////////////////////////////////////////

namespace PostProcessData
{
	struct GaussianBlurData
	{
		DirectX::XMFLOAT2 TexelSize;
		int BlurStrength;
		float Sigma;
	};
}

class PostProcessGaussianBlur : public PostProcess<PostProcessData::GaussianBlurData>
{
public:
	PostProcessGaussianBlur(bool bActive, const PostProcessData::GaussianBlurData& Data, const std::pair<int, int>& Dimensions)
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

	~PostProcessGaussianBlur()
	{
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, m_HorizontalEntry);
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, m_VerticalEntry);
	}

	void RenderControls() override
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

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override
	{
		// horizontal
		DeviceContext->PSSetShader(m_HorizontalPS, nullptr, 0u);
		DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());
		DeviceContext->PSSetShaderResources(1u, 1u, m_GaussianWeightsSRV.GetAddressOf());

		DeviceContext->PSSetConstantBuffers(0u, 1u, m_ConstantBuffer.GetAddressOf());

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

	void UpdateGaussianWeightsBuffer()
	{
		HRESULT hResult;
		D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDeviceContext()->Map(m_GaussianWeightsBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedSubresource));
		std::vector<float> GaussianWeights(m_CurrentSettings.BlurStrength + 1, 0.f);
		FillGaussianWeights(GaussianWeights);
		memcpy(MappedSubresource.pData, GaussianWeights.data(), (m_CurrentSettings.BlurStrength + 1) * sizeof(float));
		Graphics::GetSingletonPtr()->GetDeviceContext()->Unmap(m_GaussianWeightsBuffer.Get(), 0u);
	}

	float CalcGaussianWeight(int x)
	{
		float AdjustedSigma = m_CurrentSettings.Sigma + 0.2f * abs(x);
		return expf(-0.5f * (x * x) / (AdjustedSigma * AdjustedSigma));
	}

	void FillGaussianWeights(std::vector<float>& GaussianWeights)
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

private:
	const UINT m_MaxBlurStrength = 100;
	const char* m_psFilename;
	const char* m_HorizontalEntry;
	const char* m_VerticalEntry;

	ID3D11PixelShader* m_HorizontalPS;
	ID3D11PixelShader* m_VerticalPS;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_GaussianWeightsBuffer;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_IntermediateRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_IntermediateSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_GaussianWeightsSRV;
};

/////////////////////////////////////////////////////////////////////////////////

namespace PostProcessData
{
	struct PixelationData
	{
		DirectX::XMFLOAT2 TruePixelSize;
		float PixelSize;
		float Padding;
	};
}

class PostProcessPixelation : public PostProcess<PostProcessData::PixelationData>
{
public:
	PostProcessPixelation(bool bActive, const PostProcessData::PixelationData& Data)
		: PostProcess(bActive, Data, "Pixelation"), m_psFilename("Shaders/PixelationPS.hlsl")
	{
		SetupPixelShader(m_PixelShader, m_psFilename);
	}

	~PostProcessPixelation()
	{
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename);
	}

	void RenderControls() override
	{
		bool bDirty = false;
		
		if (ImGui::SliderFloat("Pixel Size", &m_CurrentSettings.PixelSize, 1.f, 32.f, "%.0f", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;

		if (bDirty)
			UpdateConstantBuffer();

		IPostProcess::RenderControls();
	}

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override
	{
		DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);
		DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());
		DeviceContext->PSSetConstantBuffers(0u, 1u, m_ConstantBuffer.GetAddressOf());

		DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);
		DeviceContext->DrawIndexed(6u, 0u, 0);
		GetProfiler()->AddDrawCall();
	}

private:
	const char* m_psFilename;
};

/////////////////////////////////////////////////////////////////////////////////

namespace PostProcessData
{
	struct BloomData
	{
		float LuminanceThreshold;
		int BlurStrength;
		float Sigma;
		float Padding;
	};
}

class PostProcessBloom : public PostProcess<PostProcessData::BloomData>
{
public:
	PostProcessBloom(bool bActive, const PostProcessData::BloomData& Data, const std::pair<int, int>& Dimensions)
		: PostProcess(bActive, Data, "Bloom", false), m_psFilename("Shaders/BloomPS.hlsl")
	{
		m_LuminanceEntry = "LuminancePS";
		m_BloomEntry = "BloomPS";
		
		HRESULT hResult;
		D3D11_BUFFER_DESC BufferDesc = {};
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4);
		BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		DirectX::XMFLOAT4 LuminanceData = DirectX::XMFLOAT4(Data.LuminanceThreshold, 0.f, 0.f, 0.f);
		D3D11_SUBRESOURCE_DATA BufferData = {};
		BufferData.pSysMem = &LuminanceData;

		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &BufferData, &m_ConstantBuffer));
		NAME_D3D_RESOURCE(m_ConstantBuffer, ("Post process " + m_Name + " constant buffer").c_str());

		SetupPixelShader(m_LuminancePS, m_psFilename, m_LuminanceEntry);
		SetupPixelShader(m_BloomPS, m_psFilename, m_BloomEntry);

		D3D11_TEXTURE2D_DESC TextureDesc = {};
		TextureDesc.Width = Dimensions.first;
		TextureDesc.Height = Dimensions.second;
		TextureDesc.MipLevels = 1;
		TextureDesc.ArraySize = 1;
		TextureDesc.SampleDesc.Count = 1;
		TextureDesc.Usage = D3D11_USAGE_DEFAULT;
		TextureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		
		ID3D11Texture2D* LuminousTexture;
		ID3D11Texture2D* BlurredTexture;
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateTexture2D(&TextureDesc, nullptr, &LuminousTexture));
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateTexture2D(&TextureDesc, nullptr, &BlurredTexture));
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateRenderTargetView(LuminousTexture, NULL, &m_LuminousRTV));
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateRenderTargetView(BlurredTexture, NULL, &m_BlurredRTV));
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateShaderResourceView(LuminousTexture, NULL, &m_LuminousSRV));
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateShaderResourceView(BlurredTexture, NULL, &m_BlurredSRV));

		NAME_D3D_RESOURCE(LuminousTexture, ("Post process " + m_Name + " luminous texture").c_str());
		NAME_D3D_RESOURCE(BlurredTexture, ("Post process " + m_Name + " blurred texture").c_str());
		NAME_D3D_RESOURCE(m_LuminousRTV, ("Post process " + m_Name + " luminous texture RTV").c_str());
		NAME_D3D_RESOURCE(m_BlurredRTV, ("Post process " + m_Name + " blurred texture RTV").c_str());
		NAME_D3D_RESOURCE(m_LuminousSRV, ("Post process " + m_Name + " luminous texture SRV").c_str());
		NAME_D3D_RESOURCE(m_BlurredSRV, ("Post process " + m_Name + " blurred texture SRV").c_str());

		LuminousTexture->Release();
		BlurredTexture->Release();

		PostProcessData::GaussianBlurData GaussianData = {};
		GaussianData.TexelSize = DirectX::XMFLOAT2(1.f / (float)Dimensions.first, 1.f / (float)Dimensions.second);
		GaussianData.BlurStrength = Data.BlurStrength;
		GaussianData.Sigma = Data.Sigma;
		m_BlurPostProcess = std::make_unique<PostProcessGaussianBlur>(true, GaussianData, Dimensions);
		m_BlurPostProcess->SetOwner(this);
		m_OwnedPostProcesses.push_back(m_BlurPostProcess.get());

		m_OnResetToDefaults.Bind(std::function<void()>([this]() { this->m_BlurPostProcess->ResetToDefaults(); }));
	}

	~PostProcessBloom()
	{
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, m_LuminanceEntry);
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, m_BloomEntry);
	}

	void RenderControls() override
	{
		bool bDirty = false;
		
		if (ImGui::SliderFloat("Luminance Threshold", &m_CurrentSettings.LuminanceThreshold, 0.f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;

		if (bDirty)
			UpdateBuffer();

		m_BlurPostProcess->RenderControls();
		IPostProcess::RenderControls();
	}

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override
	{		
		// render luminous pixels
		DeviceContext->PSSetShader(m_LuminancePS, nullptr, 0u);
		DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());
		DeviceContext->PSSetConstantBuffers(0u, 1u, m_ConstantBuffer.GetAddressOf());

		DeviceContext->OMSetRenderTargets(1u, m_LuminousRTV.GetAddressOf(), nullptr);
		DeviceContext->DrawIndexed(6u, 0u, 0);
		GetProfiler()->AddDrawCall();

		// blur luminous pixels
		m_BlurPostProcess->ApplyPostProcess(DeviceContext, m_BlurredRTV, m_LuminousSRV);

		// add bloom to original
		ID3D11RenderTargetView* NullRTVs[] = { nullptr };
		Graphics::GetSingletonPtr()->GetDeviceContext()->OMSetRenderTargets(1u, NullRTVs, nullptr);

		DeviceContext->PSSetShader(m_BloomPS, nullptr, 0u);
		DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());
		DeviceContext->PSSetShaderResources(1u, 1u, m_BlurredSRV.GetAddressOf());
		DeviceContext->PSSetConstantBuffers(0u, 1u, m_ConstantBuffer.GetAddressOf());

		DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);
		DeviceContext->DrawIndexed(6u, 0u, 0);
		GetProfiler()->AddDrawCall();
	}

	void UpdateBuffer()
	{
		HRESULT hResult;
		DirectX::XMFLOAT4 Data = DirectX::XMFLOAT4(m_CurrentSettings.LuminanceThreshold, 0.f, 0.f, 0.f);
		D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDeviceContext()->Map(m_ConstantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedSubresource));
		memcpy(MappedSubresource.pData, &Data, sizeof(DirectX::XMFLOAT4));
		Graphics::GetSingletonPtr()->GetDeviceContext()->Unmap(m_ConstantBuffer.Get(), 0u);
	}

private:
	std::unique_ptr<PostProcessGaussianBlur> m_BlurPostProcess;
	const char* m_psFilename;
	const char* m_LuminanceEntry;
	const char* m_BloomEntry;

	ID3D11PixelShader* m_LuminancePS;
	ID3D11PixelShader* m_BloomPS;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_LuminousRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_BlurredRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_LuminousSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_BlurredSRV;
};

/////////////////////////////////////////////////////////////////////////////////

namespace PostProcessData
{
	enum class ToneMapperFormula
	{
		ReinhardBasic,
		ReinhardExtended,
		ReinhardExtendedBias,
		NarkowiczACES,
		HillACES,
		None
	};
	
	struct ToneMapperData
	{
		float WhiteLevel;
		float Exposure;
		float Bias;
		ToneMapperFormula Formula;
	};
}

class PostProcessToneMapper : public PostProcess<PostProcessData::ToneMapperData>
{
	using enum PostProcessData::ToneMapperFormula;

public:	
	PostProcessToneMapper(bool bActive, const PostProcessData::ToneMapperData& Data)
		: PostProcess(bActive, Data, "Tone Mapper"), m_psFilename("Shaders/ToneMapperPS.hlsl")
	{
		assert(static_cast<int>(Data.Formula) >= 0 && Data.Formula < None);
		SetupPixelShader(m_PixelShader, m_psFilename);
	}

	~PostProcessToneMapper()
	{
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename);
	}

	void RenderControls() override
	{
		bool bDirty = false;
		const char* Formulas[] = { "Reinhard Basic", "Reinhard Extended", "Reinhard Extended Bias", "Narkowicz ACES", "Hill ACES" };
		
		if (ImGui::Combo("Formula", (int*)&m_CurrentSettings.Formula, Formulas, IM_ARRAYSIZE(Formulas)))
			bDirty = true;
		
		switch (m_CurrentSettings.Formula)
		{
		case ReinhardBasic:
			break;
		case ReinhardExtended:
			if (ImGui::SliderFloat("White Level", &m_CurrentSettings.WhiteLevel, 0.f, 10.f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
				bDirty = true;
			break;
		case ReinhardExtendedBias:
			if (ImGui::SliderFloat("Bias", &m_CurrentSettings.Bias, 0.f, 10.f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
				bDirty = true;
			break;
		case NarkowiczACES:
			break;
		case HillACES:
			break;
		default:
			break;
		}

		if (bDirty)
			UpdateConstantBuffer();

		IPostProcess::RenderControls();
	}

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override
	{
		DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);
		DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());

		DeviceContext->PSSetConstantBuffers(0u, 1u, m_ConstantBuffer.GetAddressOf());

		DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);

		DeviceContext->DrawIndexed(6u, 0u, 0);
		GetProfiler()->AddDrawCall();
	}

private:
	const char* m_psFilename;
};

/////////////////////////////////////////////////////////////////////////////////

namespace PostProcessData
{
	struct GammaCorrectionData
	{
		float Gamma;
		DirectX::XMFLOAT3 Padding = {};
	};
}

class PostProcessGammaCorrection : public PostProcess<PostProcessData::GammaCorrectionData>
{
public:
	PostProcessGammaCorrection(bool bActive, const PostProcessData::GammaCorrectionData& Data)
		: PostProcess(bActive, Data, "Gamma Correction"), m_psFilename("Shaders/GammaCorrectionPS.hlsl")
	{
		SetupPixelShader(m_PixelShader, m_psFilename);
	}

	~PostProcessGammaCorrection()
	{
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename);
	}

	void RenderControls() override
	{
		bool bDirty = false;

		if (ImGui::SliderFloat("Gamma", &m_CurrentSettings.Gamma, 1.f, 3.f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;

		if (bDirty)
			UpdateConstantBuffer();

		IPostProcess::RenderControls();
	}

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override
	{
		DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);
		DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());

		DeviceContext->PSSetConstantBuffers(0u, 1u, m_ConstantBuffer.GetAddressOf());

		DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);

		DeviceContext->DrawIndexed(6u, 0u, 0);
		GetProfiler()->AddDrawCall();
	}

private:
	const char* m_psFilename;
};

/////////////////////////////////////////////////////////////////////////////////

namespace PostProcessData
{
	struct ColorCorrectionData
	{
		float Contrast;
		float Brightness;
		float Saturation;
		float Padding;
	};
}

class PostProcessColorCorrection : public PostProcess<PostProcessData::ColorCorrectionData>
{
public:
	PostProcessColorCorrection(bool bActive, const PostProcessData::ColorCorrectionData& Data)
		: PostProcess(bActive, Data, "Color Correction"), m_psFilename("Shaders/ColorCorrectionPS.hlsl")
	{
		SetupPixelShader(m_PixelShader, m_psFilename);
	}

	~PostProcessColorCorrection()
	{
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename);
	}

	void RenderControls() override
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

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override
	{
		DeviceContext->PSSetShader(m_PixelShader, nullptr, 0u);
		DeviceContext->PSSetShaderResources(0u, 1u, SRV.GetAddressOf());

		DeviceContext->PSSetConstantBuffers(0u, 1u, m_ConstantBuffer.GetAddressOf());

		DeviceContext->OMSetRenderTargets(1u, RTV.GetAddressOf(), nullptr);

		DeviceContext->DrawIndexed(6u, 0u, 0);
		GetProfiler()->AddDrawCall();
	}

private:
	const char* m_psFilename;
};

/////////////////////////////////////////////////////////////////////////////////

namespace PostProcessData
{
	struct TemporalAAData
	{
		float Alpha;
		DirectX::XMFLOAT3 Padding = {};
	};
}

class PostProcessTemporalAA : public PostProcess<PostProcessData::TemporalAAData>
{
public:
	PostProcessTemporalAA(bool bActive, PostProcessData::TemporalAAData Data, std::shared_ptr<CameraManager>& CamManager)
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

	~PostProcessTemporalAA()
	{
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename);
	}

	void RenderControls() override
	{
		bool bDirty = false;

		if (ImGui::SliderFloat("Alpha", &m_CurrentSettings.Alpha, 0.f, 1.f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;

		if (bDirty)
			UpdateConstantBuffer();

		IPostProcess::RenderControls();
	}

private:
	void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) override
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
			ID3D11ShaderResourceView* SRVs[2] = { SRV.Get(), m_HistoryFrameSRV.Get() };
			DeviceContext->PSSetShaderResources(0u, 2u, SRVs);

			DeviceContext->PSSetConstantBuffers(0u, 1u, m_ConstantBuffer.GetAddressOf());

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

private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_HistoryFrameTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_HistoryFrameSRV;

	const char* m_psFilename;
};

#endif