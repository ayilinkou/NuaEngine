#pragma once

#ifndef INSTANCED_SHADER_H
#define INSTANCED_SHADER_H

#include <vector>

#include <d3d11.h>
#include <DirectXMath.h>

#include <wrl.h>

#include "Common.h"

class PointLight;
class DirectionalLight;

class InstancedShader
{
public:
	InstancedShader() {};
	~InstancedShader();

	bool Initialise(ID3D11Device* Device);
	void Shutdown();

	static void ActivateShaderOpaque(ID3D11DeviceContext* DeviceContext);
	static void ActivateShaderTransparent(ID3D11DeviceContext* DeviceContext);

	Microsoft::WRL::ComPtr<ID3D11InputLayout> GetInputLayout() const { return ms_InputLayout; }

private:
	bool InitialiseShader(ID3D11Device* Device);

private:
	inline static ID3D11VertexShader* ms_VertexShader			= nullptr;
	inline static ID3D11PixelShader* ms_PixelShaderOpaque		= nullptr;
	inline static ID3D11PixelShader* ms_PixelShaderTransparent	= nullptr;
	inline static Microsoft::WRL::ComPtr<ID3D11InputLayout> ms_InputLayout;

	const char* m_vsFilename = "";
	const char* m_psFilename = "";
};

#endif
