#pragma once

#ifndef SHADER_H
#define SHADER_H

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include <wrl.h>

class Shader
{
private:
	struct MatrixBuffer
	{
		DirectX::XMMATRIX WorldMatrix;
		DirectX::XMMATRIX ViewMatrix;
		DirectX::XMMATRIX ProjectionMatrix;
	};

	struct LightingBuffer
	{
		DirectX::XMFLOAT3 CameraPos;
		float Radius;
		DirectX::XMFLOAT3 LightPos;
		float SpecularPower;
		DirectX::XMFLOAT3 DiffuseColor;
		float Padding;
	};

public:
	Shader() = default;
	~Shader();

	bool Initialise(ID3D11Device* Device);
	void Shutdown();
	void ActivateShader(ID3D11DeviceContext* DeviceContext);
	bool SetShaderParameters(ID3D11DeviceContext* DeviceContext, DirectX::XMMATRIX World, DirectX::XMMATRIX View, DirectX::XMMATRIX Projection,
								DirectX::XMFLOAT3 CameraPos, float Radius, DirectX::XMFLOAT3 LightPos, DirectX::XMFLOAT3 DiffuseColor, float SpecularPower);

	Microsoft::WRL::ComPtr<ID3D11InputLayout> GetInputLayout() const { return m_InputLayout; }

private:
	bool InitialiseShader(ID3D11Device* Device, const char* vsFilename, const char* psFilename);

private:
	ID3D11VertexShader* m_VertexShader	= nullptr;
	ID3D11PixelShader* m_PixelShader	= nullptr;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_InputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_MatrixBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_LightingBuffer;

	const char* m_vsFilename = "";
	const char* m_psFilename = "";
};

#endif
