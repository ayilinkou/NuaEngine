#pragma once

#ifndef POSTPROCESS_H
#define POSTPROCESS_H

#include <string>
#include <memory>
#include <vector>

#include "DirectXMath.h"
#include "d3d11.h"
#include "wrl.h"

#include "ImGui/imgui.h"

#include "Delegate.h"
#include "Profiler.h"
#include "Graphics.h"
#include "MyMacros.h"

class PostProcessEmpty;

class IPostProcess
{
public:
	virtual ~IPostProcess() {}
	virtual void ResetToDefaults() = 0;

	virtual void RenderControls();

public:
	void ApplyPostProcess(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV);

	static void InitStatics(std::shared_ptr<Profiler> Profiler);
	static void ShutdownStatics();

	static ID3D11VertexShader* GetQuadVertexShader() { return IPostProcess::ms_QuadVertexShader; }
	static Microsoft::WRL::ComPtr<ID3D11Buffer> GetQuadVertexBuffer() {	return IPostProcess::ms_QuadVertexBuffer; }
	static Microsoft::WRL::ComPtr<ID3D11Buffer> GetQuadIndexBuffer() { return IPostProcess::ms_QuadIndexBuffer;	}
	static Microsoft::WRL::ComPtr<ID3D11InputLayout> GetQuadInputLayout() {	return IPostProcess::ms_QuadInputLayout; }
	static std::shared_ptr<Profiler>& GetProfiler() { return IPostProcess::ms_Profiler;	}
	static std::shared_ptr<PostProcessEmpty> GetEmptyPostProcess();

	Microsoft::WRL::ComPtr<ID3D11PixelShader> GetPixelShader() const { return m_PixelShader; };

	void ResetToInitialActive();

	void SetActive(bool bNewActive) { bNewActive ? Activate() : Deactivate(); }
	void Activate() { m_bActive = true; m_bRecentlyActivated = true; }
	void Deactivate() { m_bActive = false; }
	void SetOwner(IPostProcess* pOwner) { m_pOwner = pOwner; }
	bool IsActive() const { return m_bActive; }
	bool& GetIsActive() { return m_bActive; }
	const std::string& GetName() const { return m_Name; }

protected:
	IPostProcess(bool bStartActive, const std::string& Name) : m_bInitialActive(bStartActive), m_Name(Name) { SetActive(bStartActive); }
	
	virtual void ApplyPostProcessImpl(ID3D11DeviceContext* DeviceContext, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV) = 0;

	bool SetupPixelShader(ID3D11PixelShader*& PixelShader, const char* PSFilepath, const char* EntryFunc = "main");

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

/////////////////////////////////////////////////////////////////////////////////

template <typename PostProcessDataType>
class PostProcess : public IPostProcess
{
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

public:
	virtual void ResetToDefaults() override
	{
		memcpy(&m_CurrentSettings, &m_InitialSettings, sizeof(PostProcessDataType));
		UpdateConstantBuffer();
		m_OnResetToDefaults.Broadcast();
	}

protected:
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

#endif