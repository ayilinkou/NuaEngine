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

class PostProcessEmpty;

class PostProcess
{
public:
	void ApplyPostProcess(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV)
	{
		Graphics::GetSingletonPtr()->GetDeviceContext()->OMSetRenderTargets(8u, NullRTVs, nullptr);
		Graphics::GetSingletonPtr()->GetDeviceContext()->PSSetShaderResources(0u, 8u, NullSRVs);
		ApplyPostProcessImpl(DeviceContext, RTV, SRV);
	}

	virtual ~PostProcess() {}

	virtual void RenderControls()
	{
		ImGui::Text("Controls not set up for this post process!");
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
		return PostProcess::ms_QuadVertexShader;
	}

	static Microsoft::WRL::ComPtr<ID3D11Buffer> GetQuadVertexBuffer()
	{
		return PostProcess::ms_QuadVertexBuffer;
	}

	static Microsoft::WRL::ComPtr<ID3D11Buffer> GetQuadIndexBuffer()
	{
		return PostProcess::ms_QuadIndexBuffer;
	}

	static Microsoft::WRL::ComPtr<ID3D11InputLayout> GetQuadInputLayout()
	{
		return PostProcess::ms_QuadInputLayout;
	}

	static std::shared_ptr<Profiler>& GetProfiler()
	{
		return PostProcess::ms_Profiler;
	}

	static std::shared_ptr<PostProcessEmpty> GetEmptyPostProcess()
	{
		if (ms_EmptyPostProcess.get())
		{
			return PostProcess::ms_EmptyPostProcess;
		}

		ms_EmptyPostProcess = std::make_shared<PostProcessEmpty>();
		return PostProcess::ms_EmptyPostProcess;
	}

	Microsoft::WRL::ComPtr<ID3D11PixelShader> GetPixelShader() const { return m_PixelShader; };

	void Activate() { m_bActive = true; m_bRecentlyActivated = true; }
	void Deactivate() { m_bActive = false; }
	bool IsActive() const { return m_bActive; }
	bool& GetIsActive() { return m_bActive; }
	const std::string& GetName() const { return m_Name; }

protected:
	struct Vertex
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT2 TexCoord;
	};
	
	ID3D11PixelShader* m_PixelShader = nullptr;
	bool m_bActive = true;
	bool m_bRecentlyActivated = true;
	std::string m_Name = "";
	
	virtual void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) = 0;

	bool SetupPixelShader(ID3D11PixelShader*& PixelShader, const char* PSFilepath, const char* EntryFunc = "main")
	{		
		PixelShader = ResourceManager::GetSingletonPtr()->LoadShader<ID3D11PixelShader>(PSFilepath, EntryFunc);
		assert(PixelShader);

		return true;
	}

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


/////////////////////////////////////////////////////////////////////////////////

class PostProcessEmpty : public PostProcess
{
public:
	PostProcessEmpty()
	{
		m_Name = "Empty";
		m_psFilename = "Shaders/QuadPS.hlsl";
		SetupPixelShader(m_PixelShader, m_psFilename);
	}

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

class PostProcessFog : public PostProcess
{
private:
	struct FogData
	{
		DirectX::XMFLOAT3 FogColor;
		int Formula;
		float Density;
		float NearPlane;
		float FarPlane;
		float Padding;
	};

public:
	enum FogFormula
	{
		Linear,
		Exponential,
		ExponentialSquared,
		None
	};

public:
	PostProcessFog(float r, float g, float b, float Density, FogFormula Formula)
	{
		assert(Formula >= 0 && Formula < FogFormula::None);
		m_Name = "Fog";
		m_psFilename = "Shaders/FogPS.hlsl";

		HRESULT hResult;
		D3D11_BUFFER_DESC BufferDesc = {};
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.ByteWidth = sizeof(FogData);
		BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		m_FogData.FogColor = { r, g, b };
		m_FogData.Formula = Formula;
		m_FogData.Density = Density;
		m_FogData.NearPlane = Graphics::GetSingletonPtr()->GetNearPlane();
		m_FogData.FarPlane = Graphics::GetSingletonPtr()->GetFarPlane();
		m_FogData.Padding = 0.f;
		D3D11_SUBRESOURCE_DATA BufferData = {};
		BufferData.pSysMem = &m_FogData;

		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &BufferData, &m_ConstantBuffer));
		NAME_D3D_RESOURCE(m_ConstantBuffer, ("Post process " + m_Name + " constant buffer").c_str());

		SetupPixelShader(m_PixelShader, m_psFilename);
	}

	~PostProcessFog()
	{
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, "main");
	}

	virtual void RenderControls()
	{
		bool bDirty = false;
		
		if (ImGui::ColorEdit3("Fog Color", reinterpret_cast<float*>(&m_FogData.FogColor)))
			bDirty = true;

		const char* Formulas[] = { "Linear", "Exponential", "Exponential Squared" };
		if (ImGui::Combo("Formula", &m_FogData.Formula, Formulas, IM_ARRAYSIZE(Formulas)))
			bDirty = true;

		switch (m_FogData.Formula)
		{
		case Linear:
			break;
		case Exponential:
			if (ImGui::SliderFloat("Density", &m_FogData.Density, 0.f, 0.02f, "%.4f", ImGuiSliderFlags_AlwaysClamp))
				bDirty = true;
			break;
		case ExponentialSquared:
			if (ImGui::SliderFloat("Density", &m_FogData.Density, 0.f, 0.02f, "%.4f", ImGuiSliderFlags_AlwaysClamp))
				bDirty = true;
			break;
		default:
			break;
		}

		if (bDirty)
			UpdateBuffer();
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

	void UpdateBuffer()
	{
		HRESULT hResult;
		D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDeviceContext()->Map(m_ConstantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedSubresource));
		memcpy(MappedSubresource.pData, &m_FogData, sizeof(FogData));
		Graphics::GetSingletonPtr()->GetDeviceContext()->Unmap(m_ConstantBuffer.Get(), 0u);
	}

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ConstantBuffer;

	FogData m_FogData;
	const char* m_psFilename;

};

/////////////////////////////////////////////////////////////////////////////////

class PostProcessBoxBlur : public PostProcess
{
private:
	struct BlurData
	{
		DirectX::XMFLOAT2 TexelSize;
		int BlurStrength;
		float Padding;
	};

public:
	PostProcessBoxBlur(int BlurStrength)
	{
		assert(BlurStrength > 0);
		m_Name = "Box Blur";
		m_psFilename = "Shaders/BoxBlurPS.hlsl";
		m_HorizontalEntry = "HorizontalPS";
		m_VerticalEntry = "VerticalPS";
		
		HRESULT hResult;
		D3D11_BUFFER_DESC BufferDesc = {};
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.ByteWidth = sizeof(BlurData);
		BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		std::pair<int, int> Dimensions = Graphics::GetSingletonPtr()->GetRenderTargetDimensions();
		m_BlurData = {};
		m_BlurData.TexelSize = DirectX::XMFLOAT2(1.f / (float)Dimensions.first, 1.f / (float)Dimensions.second);
		m_BlurData.BlurStrength = BlurStrength;

		D3D11_SUBRESOURCE_DATA BufferData = {};
		BufferData.pSysMem = &m_BlurData;

		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &BufferData, &m_ConstantBuffer));
		NAME_D3D_RESOURCE(m_ConstantBuffer, ("Post process " + m_Name + " constant buffer").c_str());

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

	~PostProcessBoxBlur()
	{
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, m_HorizontalEntry);
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, m_VerticalEntry);
	}

	void RenderControls() override
	{
		bool bDirty = false;
		
		ImGui::Text("Blur Strength is currently hard coded in the shader. Fix it eventually.");
		if (ImGui::SliderInt("Blur Strength", &m_BlurData.BlurStrength, 1, 32, "%.d", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;

		if (bDirty)
			UpdateBuffer();
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

	void UpdateBuffer()
	{
		HRESULT hResult;
		D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDeviceContext()->Map(m_ConstantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedSubresource));
		memcpy(MappedSubresource.pData, &m_BlurData, sizeof(BlurData));
		Graphics::GetSingletonPtr()->GetDeviceContext()->Unmap(m_ConstantBuffer.Get(), 0u);
	}

private:
	ID3D11PixelShader* m_HorizontalPS;
	ID3D11PixelShader* m_VerticalPS;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_IntermediateRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_IntermediateSRV;

	BlurData m_BlurData;
	const char* m_psFilename;
	const char* m_HorizontalEntry;
	const char* m_VerticalEntry;

};

/////////////////////////////////////////////////////////////////////////////////

class PostProcessGaussianBlur : public PostProcess
{
private:
	struct BlurData
	{
		DirectX::XMFLOAT2 TexelSize;
		int BlurStrength;
		float Sigma;
	};

public:
	PostProcessGaussianBlur(int BlurStrength, float Sigma)
	{
		assert(BlurStrength > 0 && (UINT)BlurStrength <= m_MaxBlurStrength);
		m_Name = "Gaussian Blur";
		m_psFilename = "Shaders/GaussianBlurPS.hlsl";
		m_HorizontalEntry = "HorizontalPS";
		m_VerticalEntry = "VerticalPS";

		HRESULT hResult;
		D3D11_BUFFER_DESC BufferDesc = {};
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4);
		BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		std::pair<int, int> Dimensions = Graphics::GetSingletonPtr()->GetRenderTargetDimensions();
		m_BlurData = {};
		m_BlurData.TexelSize = DirectX::XMFLOAT2(1.f / (float)Dimensions.first, 1.f / (float)Dimensions.second);
		m_BlurData.BlurStrength = BlurStrength;
		m_BlurData.Sigma = Sigma;

		D3D11_SUBRESOURCE_DATA BufferData = {};
		BufferData.pSysMem = &m_BlurData;

		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &BufferData, &m_ConstantBuffer));
		NAME_D3D_RESOURCE(m_ConstantBuffer, ("Post process " + m_Name + " constant buffer").c_str());

		std::vector<float> GaussianWeights(m_MaxBlurStrength + 1, 0.f);
		FillGaussianWeights(GaussianWeights);

		BufferDesc = {};
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.ByteWidth = sizeof(float) * ((UINT)m_MaxBlurStrength + 1);
		BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		BufferDesc.StructureByteStride = sizeof(float);
		BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

		BufferData = {};
		BufferData.pSysMem = GaussianWeights.data();

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
		
		if (ImGui::SliderInt("Blur Strength", &m_BlurData.BlurStrength, 0, m_MaxBlurStrength, "%.d", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;
		if (ImGui::SliderFloat("Sigma", &m_BlurData.Sigma, 1.f, 8.f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;

		if (bDirty)
			UpdateBuffers();
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

	void UpdateBuffers()
	{
		HRESULT hResult;
		D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDeviceContext()->Map(m_ConstantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedSubresource));
		memcpy(MappedSubresource.pData, &m_BlurData, sizeof(BlurData));
		Graphics::GetSingletonPtr()->GetDeviceContext()->Unmap(m_ConstantBuffer.Get(), 0u);

		MappedSubresource = {};
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDeviceContext()->Map(m_GaussianWeightsBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedSubresource));
		std::vector<float> GaussianWeights(m_BlurData.BlurStrength + 1, 0.f);
		FillGaussianWeights(GaussianWeights);
		memcpy(MappedSubresource.pData, GaussianWeights.data(), (m_BlurData.BlurStrength + 1) * sizeof(float));
		Graphics::GetSingletonPtr()->GetDeviceContext()->Unmap(m_GaussianWeightsBuffer.Get(), 0u);
	}

	float CalcGaussianWeight(int x)
	{
		float AdjustedSigma = m_BlurData.Sigma + 0.2f * abs(x);
		return expf(-0.5f * (x * x) / (AdjustedSigma * AdjustedSigma));
	}

	void FillGaussianWeights(std::vector<float>& GaussianWeights)
	{
		float Sum = 0.f;
		for (int i = 0; i <= m_BlurData.BlurStrength; i++)
		{
			GaussianWeights[i] = CalcGaussianWeight(i);
			Sum += GaussianWeights[i];
		}

		// normalise the weights so that they sum to 1
		for (int i = 0; i <= m_BlurData.BlurStrength; i++)
		{
			GaussianWeights[i] /= Sum;
		}
	}

private:
	BlurData m_BlurData;
	const UINT m_MaxBlurStrength = 100;
	const char* m_psFilename;
	const char* m_HorizontalEntry;
	const char* m_VerticalEntry;

	ID3D11PixelShader* m_HorizontalPS;
	ID3D11PixelShader* m_VerticalPS;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_GaussianWeightsBuffer;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_IntermediateRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_IntermediateSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_GaussianWeightsSRV;
};

/////////////////////////////////////////////////////////////////////////////////

class PostProcessPixelation : public PostProcess
{
public:
	PostProcessPixelation(float pixelSize) : m_PixelSize(pixelSize)
	{
		m_Name = "Pixelation";
		m_psFilename = "Shaders/PixelationPS.hlsl";
		
		HRESULT hResult;
		D3D11_BUFFER_DESC BufferDesc = {};
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4);
		BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		std::pair<int, int> Dimensions = Graphics::GetSingletonPtr()->GetRenderTargetDimensions();
		DirectX::XMFLOAT4 Data = DirectX::XMFLOAT4(1.f / (float)Dimensions.first, 1.f / (float)Dimensions.second, m_PixelSize, 0.f);
		D3D11_SUBRESOURCE_DATA BufferData = {};
		BufferData.pSysMem = &Data;

		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &BufferData, &m_ConstantBuffer));
		NAME_D3D_RESOURCE(m_ConstantBuffer, ("Post process " + m_Name + " constant buffer").c_str());

		SetupPixelShader(m_PixelShader, m_psFilename);
	}

	~PostProcessPixelation()
	{
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename);
	}

	void RenderControls() override
	{
		bool bDirty = false;
		
		if (ImGui::SliderFloat("Pixel Size", &m_PixelSize, 1.f, 32.f, "%.0f", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;

		if (bDirty)
			UpdateBuffer();
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

	void UpdateBuffer()
	{
		HRESULT hResult;
		std::pair<int, int> Dimensions = Graphics::GetSingletonPtr()->GetRenderTargetDimensions();
		DirectX::XMFLOAT4 Data = DirectX::XMFLOAT4(1.f / (float)Dimensions.first, 1.f / (float)Dimensions.second, m_PixelSize, 0.f);
		D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDeviceContext()->Map(m_ConstantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedSubresource));
		memcpy(MappedSubresource.pData, &Data, sizeof(DirectX::XMFLOAT4));
		Graphics::GetSingletonPtr()->GetDeviceContext()->Unmap(m_ConstantBuffer.Get(), 0u);
	}

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ConstantBuffer;

	float m_PixelSize;
	const char* m_psFilename;
};

/////////////////////////////////////////////////////////////////////////////////

class PostProcessBloom : public PostProcess
{
public:
	PostProcessBloom(float LuminanceThreshold, int BlurStrength, float Sigma) : m_LuminanceThreshold(LuminanceThreshold)
	{
		m_Name = "Bloom";
		m_psFilename = "Shaders/BloomPS.hlsl";
		m_LuminanceEntry = "LuminancePS";
		m_BloomEntry = "BloomPS";
		
		HRESULT hResult;
		D3D11_BUFFER_DESC BufferDesc = {};
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4);
		BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		DirectX::XMFLOAT4 Data = DirectX::XMFLOAT4(m_LuminanceThreshold, 0.f, 0.f, 0.f);
		D3D11_SUBRESOURCE_DATA BufferData = {};
		BufferData.pSysMem = &Data;

		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &BufferData, &m_ConstantBuffer));
		NAME_D3D_RESOURCE(m_ConstantBuffer, ("Post process " + m_Name + " constant buffer").c_str());

		SetupPixelShader(m_LuminancePS, m_psFilename, m_LuminanceEntry);
		SetupPixelShader(m_BloomPS, m_psFilename, m_BloomEntry);

		std::pair<int, int> Dimensions = Graphics::GetSingletonPtr()->GetRenderTargetDimensions();
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

		m_BlurPostProcess = std::make_unique<PostProcessGaussianBlur>(BlurStrength, Sigma);
	}

	~PostProcessBloom()
	{
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, m_LuminanceEntry);
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename, m_BloomEntry);
	}

	void RenderControls() override
	{
		bool bDirty = false;
		
		if (ImGui::SliderFloat("Luminance Threshold", &m_LuminanceThreshold, 0.f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;

		if (bDirty)
			UpdateBuffer();

		m_BlurPostProcess->RenderControls();
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
		DirectX::XMFLOAT4 Data = DirectX::XMFLOAT4(m_LuminanceThreshold, 0.f, 0.f, 0.f);
		D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDeviceContext()->Map(m_ConstantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedSubresource));
		memcpy(MappedSubresource.pData, &Data, sizeof(DirectX::XMFLOAT4));
		Graphics::GetSingletonPtr()->GetDeviceContext()->Unmap(m_ConstantBuffer.Get(), 0u);
	}

private:
	float m_LuminanceThreshold;
	std::unique_ptr<PostProcess> m_BlurPostProcess;
	const char* m_psFilename;
	const char* m_LuminanceEntry;
	const char* m_BloomEntry;

	ID3D11PixelShader* m_LuminancePS;
	ID3D11PixelShader* m_BloomPS;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_LuminousRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_BlurredRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_LuminousSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_BlurredSRV;
};

/////////////////////////////////////////////////////////////////////////////////

class PostProcessToneMapper : public PostProcess
{
private:
	struct ToneMapperData {
		float WhiteLevel;
		float Exposure;
		float Bias;
		int Formula;
	};

public:
	enum ToneMapperFormula {
		ReinhardBasic,
		ReinhardExtended,
		ReinhardExtendedBias,
		NarkowiczACES,
		HillACES,
		None
	};
	
	PostProcessToneMapper(float WhiteLevel, float Exposure, float Bias, ToneMapperFormula Formula)
	{
		assert(Formula >= 0 && Formula < ToneMapperFormula::None);
		m_Name = "Tone Mapper";
		m_psFilename = "Shaders/ToneMapperPS.hlsl";
		
		HRESULT hResult;
		D3D11_BUFFER_DESC BufferDesc = {};
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4);
		BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		m_ToneMapperData = { WhiteLevel, Exposure, Bias, Formula };
		D3D11_SUBRESOURCE_DATA BufferData = {};
		BufferData.pSysMem = &m_ToneMapperData;

		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &BufferData, &m_ConstantBuffer));
		NAME_D3D_RESOURCE(m_ConstantBuffer, ("Post process " + m_Name + " constant buffer").c_str());

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
		
		if (ImGui::Combo("Formula", &m_ToneMapperData.Formula, Formulas, IM_ARRAYSIZE(Formulas)))
			bDirty = true;
		
		switch (m_ToneMapperData.Formula)
		{
		case ReinhardBasic:
			break;
		case ReinhardExtended:
			if (ImGui::SliderFloat("White Level", &m_ToneMapperData.WhiteLevel, 0.f, 10.f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
				bDirty = true;
			break;
		case ReinhardExtendedBias:
			if (ImGui::SliderFloat("Bias", &m_ToneMapperData.Bias, 0.f, 10.f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
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
			UpdateBuffer();
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

	void UpdateBuffer()
	{
		HRESULT hResult;
		D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDeviceContext()->Map(m_ConstantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedSubresource));
		memcpy(MappedSubresource.pData, &m_ToneMapperData, sizeof(ToneMapperData));
		Graphics::GetSingletonPtr()->GetDeviceContext()->Unmap(m_ConstantBuffer.Get(), 0u);
	}

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ConstantBuffer;

	ToneMapperData m_ToneMapperData;
	const char* m_psFilename;

};

/////////////////////////////////////////////////////////////////////////////////

class PostProcessGammaCorrection : public PostProcess
{
public:
	PostProcessGammaCorrection(float Gamma) : m_Gamma(Gamma)
	{
		m_Name = "Gamma Correction";
		m_psFilename = "Shaders/GammaCorrectionPS.hlsl";
		
		HRESULT hResult;
		D3D11_BUFFER_DESC BufferDesc = {};
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4);
		BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		DirectX::XMFLOAT4 Data = { m_Gamma, 0.f, 0.f, 0.f };
		D3D11_SUBRESOURCE_DATA BufferData = {};
		BufferData.pSysMem = &Data;

		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &BufferData, &m_ConstantBuffer));
		NAME_D3D_RESOURCE(m_ConstantBuffer, ("Post process " + m_Name + " constant buffer").c_str());
		
		SetupPixelShader(m_PixelShader, m_psFilename);
	}

	~PostProcessGammaCorrection()
	{
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename);
	}

	void RenderControls() override
	{
		bool bDirty = false;

		if (ImGui::SliderFloat("Gamma", &m_Gamma, 1.f, 3.f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;

		if (bDirty)
			UpdateBuffer();
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

	void UpdateBuffer()
	{
		HRESULT hResult;
		DirectX::XMFLOAT4 Data = DirectX::XMFLOAT4(m_Gamma, 0.f, 0.f, 0.f);
		D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDeviceContext()->Map(m_ConstantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedSubresource));
		memcpy(MappedSubresource.pData, &Data, sizeof(DirectX::XMFLOAT4));
		Graphics::GetSingletonPtr()->GetDeviceContext()->Unmap(m_ConstantBuffer.Get(), 0u);
	}

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ConstantBuffer;

	float m_Gamma;
	const char* m_psFilename;

};

/////////////////////////////////////////////////////////////////////////////////

class PostProcessColorCorrection : public PostProcess
{
private:
	struct ColorData
	{
		float Contrast;
		float Brightness;
		float Saturation;
		float Padding;
	};

public:
	PostProcessColorCorrection(float Contrast, float Brightness, float Saturation)
	{
		m_Name = "Color Correction";
		m_psFilename = "Shaders/ColorCorrectionPS.hlsl";

		HRESULT hResult;
		D3D11_BUFFER_DESC BufferDesc = {};
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.ByteWidth = sizeof(ColorData);
		BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		std::pair<int, int> Dimensions = Graphics::GetSingletonPtr()->GetRenderTargetDimensions();
		m_ColorData = { Contrast, Brightness, Saturation, 0.f };
		D3D11_SUBRESOURCE_DATA BufferData = {};
		BufferData.pSysMem = &m_ColorData;

		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDevice()->CreateBuffer(&BufferDesc, &BufferData, &m_ConstantBuffer));
		NAME_D3D_RESOURCE(m_ConstantBuffer, ("Post process " + m_Name + " constant buffer").c_str());

		SetupPixelShader(m_PixelShader, m_psFilename);
	}

	~PostProcessColorCorrection()
	{
		ResourceManager::GetSingletonPtr()->UnloadShader<ID3D11PixelShader>(m_psFilename);
	}

	void RenderControls() override
	{
		bool bDirty = false;
		
		if (ImGui::SliderFloat("Contrast", &m_ColorData.Contrast, 0.f, 2.f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;
		if (ImGui::SliderFloat("Brightness", &m_ColorData.Brightness, -0.5f, 0.5f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;
		if (ImGui::SliderFloat("Saturation", &m_ColorData.Saturation, 0.f, 5.f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;

		if (bDirty)
			UpdateBuffer();
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

	void UpdateBuffer()
	{
		HRESULT hResult;
		D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDeviceContext()->Map(m_ConstantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedSubresource));
		memcpy(MappedSubresource.pData, &m_ColorData, sizeof(ColorData));
		Graphics::GetSingletonPtr()->GetDeviceContext()->Unmap(m_ConstantBuffer.Get(), 0u);
	}

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ConstantBuffer;

	ColorData m_ColorData;
	const char* m_psFilename;

};

class PostProcessTemporalAA : public PostProcess
{
private:
	struct AAData
	{
		float Alpha = 0.8f;
		DirectX::XMFLOAT3 Padding = {};
	};

public:
	PostProcessTemporalAA(float Alpha, std::shared_ptr<CameraManager>& CamManager)
	{
		m_Name = "Temporal AA";
		m_psFilename = "Shaders/TemporalAA_PS.hlsl";

		HRESULT hResult;
		D3D11_BUFFER_DESC BufferDesc = {};
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.ByteWidth = sizeof(AAData);
		BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		std::pair<int, int> Dimensions = Graphics::GetSingletonPtr()->GetRenderTargetDimensions();
		m_AAData.Alpha = Alpha;
		D3D11_SUBRESOURCE_DATA BufferData = {};
		BufferData.pSysMem = &m_AAData;

		ID3D11Device* pDevice = Graphics::GetSingletonPtr()->GetDevice();

		ASSERT_NOT_FAILED(pDevice->CreateBuffer(&BufferDesc, &BufferData, &m_ConstantBuffer));
		NAME_D3D_RESOURCE(m_ConstantBuffer, ("Post process " + m_Name + " constant buffer").c_str());

		Microsoft::WRL::ComPtr<ID3D11Resource> TexResource;
		Graphics::GetSingletonPtr()->m_PostProcessSRVFirst->GetResource(&TexResource);
		assert(TexResource.Get());

		Microsoft::WRL::ComPtr<ID3D11Texture2D> PostProcessTexture;
		ASSERT_NOT_FAILED(TexResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&PostProcessTexture));

		D3D11_TEXTURE2D_DESC TextureDesc;
		PostProcessTexture->GetDesc(&TextureDesc);

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

		if (ImGui::SliderFloat("Alpha", &m_AAData.Alpha, 0.f, 1.f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
			bDirty = true;

		if (bDirty)
			UpdateBuffer();
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

	void UpdateBuffer()
	{
		HRESULT hResult;
		D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
		ASSERT_NOT_FAILED(Graphics::GetSingletonPtr()->GetDeviceContext()->Map(m_ConstantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &MappedSubresource));
		memcpy(MappedSubresource.pData, &m_AAData, sizeof(AAData));
		Graphics::GetSingletonPtr()->GetDeviceContext()->Unmap(m_ConstantBuffer.Get(), 0u);
	}

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_HistoryFrameTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_HistoryFrameSRV;

	AAData m_AAData;
	const char* m_psFilename;

};

#endif