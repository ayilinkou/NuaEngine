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
private:
	struct PointLightData
	{
		float Radius;
		DirectX::XMFLOAT3 LightPos;
		float SpecularPower;
		DirectX::XMFLOAT3 LightColor;
	};

	struct DirectionalLightData
	{
		DirectX::XMFLOAT3 LightDir;
		float SpecularPower;
		DirectX::XMFLOAT3 LightColor;
		float Padding = 0.f;
	};

	struct LightingBuffer
	{
		PointLightData PointLights[MAX_POINT_LIGHTS];
		DirectionalLightData DirLights[MAX_DIRECTIONAL_LIGHTS];
		DirectX::XMFLOAT3 SkylightColor = { 1.f, 1.f, 1.f };
		int PointLightCount = 0;
		int DirLightCount = 0;
		DirectX::XMFLOAT3 Padding;
	};

public:
	InstancedShader() {};
	~InstancedShader();

	bool Initialise(ID3D11Device* Device);
	void Shutdown();

	void ActivateShader(ID3D11DeviceContext* DeviceContext);
	bool SetShaderParameters(ID3D11DeviceContext* DeviceContext, const std::vector<PointLight*>& PointLights,
		const std::vector<DirectionalLight*>& DirLights, const DirectX::XMFLOAT3& SkylightColor);

	Microsoft::WRL::ComPtr<ID3D11InputLayout> GetInputLayout() const { return m_InputLayout; }

private:
	bool InitialiseShader(ID3D11Device* Device);

private:
	ID3D11VertexShader* m_VertexShader	= nullptr;
	ID3D11PixelShader* m_PixelShader	= nullptr;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_InputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_LightingBuffer;

	const char* m_vsFilename = "";
	const char* m_psFilename = "";
};

#endif
