#pragma once

#ifndef GRAPHICS_H
#define GRAPHICS_H

// LINKING //
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "assimp-vc143-mt.lib")

#include <d3d11.h>
#include <DirectXMath.h>

#include <wrl.h>

#include <utility>

struct GlobalCBuffer
{
	DirectX::XMMATRIX CurrView;
	DirectX::XMMATRIX CurrProj;
	DirectX::XMMATRIX CurrViewProj;
	DirectX::XMMATRIX CurrProjJittered;
	DirectX::XMMATRIX CurrViewProjJittered;
	DirectX::XMMATRIX PrevView;
	DirectX::XMMATRIX PrevProj;
	DirectX::XMMATRIX PrevViewProj;
	DirectX::XMMATRIX PrevProjJittered;
	DirectX::XMMATRIX PrevViewProjJittered;
	DirectX::XMFLOAT3 CameraPos;
	float CurrTime;
	float PrevTime;
	float NearZ;
	float FarZ;
	float Padding0;
	DirectX::XMFLOAT2 ScreenRes;
	DirectX::XMFLOAT2 Padding1;
};

class Graphics
{
private:
	static Graphics* m_Instance;

	Graphics();

public:
	static Graphics* GetSingletonPtr();

	bool Initialise(int ScreenWidth, int ScreenHeight, bool VSync, HWND hwnd, bool Fullscreen, float ScreenDepth, float ScreenNear);
	void Shutdown();

	void BeginScene(float Red, float Green, float Blue, float Alpha);
	void EndScene();

	void GetVideoCardInfo(char* CardName, int& Memory);

	void SetBackBufferRenderTarget();
	void EnableDepthWrite();
	void DisableDepthWrite();
	void DisableDepthWriteAlwaysPass();
	void EnableBlending();
	void DisableBlending();
	void ResetViewport();
	void SetRasterStateBackFaceCull(bool bShouldCull);
	void SetWireframeRasterState();

	bool CreateGlobalConstantBuffer();
	void UpdateGlobalConstantBuffer(const GlobalCBuffer& NewGlobalCBufferData);

private:
	bool m_VSync_Enabled;
	int m_VideoCardMemory;
	char m_VideoCardDescription[128];
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_SwapChain;
	Microsoft::WRL::ComPtr<ID3D11Device> m_Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_DeviceContext;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_BackBufferRTV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_VelocityRTV;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_DepthStencilBuffer;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_VelocityBuffer;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_DepthStencilStateWriteEnabled;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_DepthStencilStateWriteDisabled;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_DepthStencilStateWriteDisabledAlwaysPass;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_DepthStencilView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_DepthStencilSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_VelocitySRV;
	Microsoft::WRL::ComPtr<ID3D11BlendState> m_BlendStateOpaque;
	Microsoft::WRL::ComPtr<ID3D11BlendState> m_BlendStateTransparent;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_RasterStateBackFaceCullOn;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_RasterStateBackFaceCullOff;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_WireframeRasterState;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_SamplerState;
	Microsoft::WRL::ComPtr<ID3D11Query> m_PipelineStatsQuery;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_GlobalCBuffer;

	D3D11_VIEWPORT m_Viewport;

	std::pair<int, int> m_Dimensions;
	float m_NearPlane;
	float m_FarPlane;
	float m_ScreenAspect;
	GlobalCBuffer m_GlobalCBufferData;

public:
	ID3D11Device* GetDevice() const { return m_Device.Get(); }
	ID3D11DeviceContext* GetDeviceContext() const { return m_DeviceContext.Get(); }

	ID3D11DepthStencilView* GetDepthStencilView() const { return m_DepthStencilView.Get(); }
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetDepthStencilSRV() const { return m_DepthStencilSRV; }
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetVelocitySRV() const { return m_VelocitySRV; }
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> GetVelocityRTV() const { return m_VelocityRTV; }
	Microsoft::WRL::ComPtr<ID3D11SamplerState> GetSamplerState() const { return m_SamplerState; }

	std::pair<int, int> GetRenderTargetDimensions() const { return m_Dimensions; }
	float GetNearPlane() const { return m_NearPlane; }
	float GetFarPlane() const { return m_FarPlane; }

	DirectX::XMMATRIX GetDefaultProjMatrix() const;

	Microsoft::WRL::ComPtr<ID3D11Query> GetPipelineStatsQuery() { return m_PipelineStatsQuery; }

public:
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_PostProcessRTVFirst;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_PostProcessRTVSecond;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_PostProcessSRVFirst;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_PostProcessSRVSecond;
};

#endif
