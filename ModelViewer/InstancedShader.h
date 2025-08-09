#pragma once

#ifndef INSTANCED_SHADER_H
#define INSTANCED_SHADER_H

#include <vector>

#include <d3d11.h>
#include <d3dcompiler.h>
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

	void ActivateShader(ID3D11DeviceContext* DeviceContext);

	Microsoft::WRL::ComPtr<ID3D11InputLayout> GetInputLayout() const { return m_InputLayout; }

private:
	bool InitialiseShader(ID3D11Device* Device);

private:
	ID3D11VertexShader* m_VertexShader	= nullptr;
	ID3D11PixelShader* m_PixelShader	= nullptr;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_InputLayout;

	const char* m_vsFilename = "";
	const char* m_psFilename = "";
};

#endif
