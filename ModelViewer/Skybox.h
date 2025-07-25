#pragma once

#ifndef SKYBOX_H
#define SKYBOX_H

#include <vector>
#include <string>
#include <memory>

#include "wrl.h"

#include "d3d11.h"
#include "DirectXMath.h"

class CameraManager;
class Profiler;

class Skybox
{
public:
	Skybox() {}
	~Skybox();

	bool Init(std::shared_ptr<CameraManager> CamManager, std::shared_ptr<Profiler> pProfiler);
	void Render();
	void Shutdown();

	const DirectX::XMFLOAT3& GetAverageSkyColor() const { return m_AverageSkyColor; }

private:
	bool LoadTextures();
	bool CreateBuffers();

	void CalculateAverageSkyColor(const std::vector<std::vector<BYTE>>& TextureData);

private:
	std::string m_TexturesDir = "Textures/skybox/";
	std::vector<std::string> m_FileNames { "right.jpg", "left.jpg", "top.jpg", "bottom.jpg", "front.jpg", "back.jpg" };
	std::vector<ID3D11Texture2D*> m_Textures;

	DirectX::XMFLOAT3 m_AverageSkyColor = { 1.f, 1.f, 1.f };

	ID3D11VertexShader* m_VertexShader	= nullptr;
	ID3D11PixelShader* m_PixelShader	= nullptr;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_SRV;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_CubeTexture;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_InputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_VertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_IndexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ConstantBuffer;

	const char* m_vsFilename = "";
	const char* m_psFilename = "";

	std::shared_ptr<CameraManager> m_CameraManager;
	std::shared_ptr<Profiler> m_Profiler;
};

#endif
