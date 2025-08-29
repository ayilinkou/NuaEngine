#pragma once

#ifndef DEFERRED_SHADER_H
#define DEFERRED_SHADER_H

#include <vector>

#include <d3d11.h>
#include <DirectXMath.h>

#include <wrl.h>

class DeferredShader
{
public:
	DeferredShader() {};
	~DeferredShader();

	bool Initialise(ID3D11Device* Device);
	void Shutdown();

	static void ActivateGeoPassShader(ID3D11DeviceContext* pContext);
	static void ActivateLightingPassShader(ID3D11DeviceContext* pContext);

	Microsoft::WRL::ComPtr<ID3D11InputLayout> GetInputLayout() const { return ms_InputLayout; }

private:
	bool InitialiseShader(ID3D11Device* Device);

private:
	inline static ID3D11VertexShader* ms_VertexShader = nullptr;
	inline static ID3D11PixelShader* ms_GeoPassPixelShader = nullptr;
	inline static ID3D11PixelShader* ms_LightingPixelShader = nullptr;
	inline static Microsoft::WRL::ComPtr<ID3D11InputLayout> ms_InputLayout;

	const char* m_vsFilename = "";
	const char* m_psFilenameGeo = "";
	const char* m_psFilenameLighting = "";
};

#endif
