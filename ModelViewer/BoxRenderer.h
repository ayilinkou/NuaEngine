#pragma once

#ifndef BOX_RENDERER_H
#define BOX_RENDERER_H

#include <memory>
#include <vector>
#include <array>

#include "d3d11.h"
#include "DirectXMath.h"

#include "wrl.h"

#include "Common.h"

class Camera;
class CameraManager;
class Profiler;
struct AABB;

class BoxRenderer
{
private:
	struct CornersBuffer
	{
		DirectX::XMFLOAT4 Corners[MAX_INSTANCE_COUNT * 8] = {};
	};

	struct CameraBuffer
	{
		DirectX::XMMATRIX ViewProj;
	};

public:
	BoxRenderer() {}
	~BoxRenderer();

	bool Init(std::shared_ptr<CameraManager> CamManager, std::shared_ptr<Profiler> pProfiler);
	void Shutdown();
	void ClearBoxes();

	void LoadBoxCorners(const AABB& BBox, const DirectX::XMMATRIX& Transform);
	void LoadFrustumCorners(const std::shared_ptr<Camera>& pCamera);

	void Render();

private:
	bool CreateShaders();
	bool CreateBuffers();
	bool CreateViews();

	void UpdateBuffers();
	void UpdateCornersBuffer(const UINT StartInstance);

private:
	ID3D11VertexShader* m_VertexShader	= nullptr;
	ID3D11PixelShader* m_PixelShader	= nullptr;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_InputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_IndexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_VertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_CornersBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_CameraCBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_CornersSRV;

	std::vector<std::array<DirectX::XMFLOAT4, 8>> m_Boxes;

	std::shared_ptr<CameraManager> m_CameraManager;
	std::shared_ptr<Profiler> m_Profiler;

	const char* m_vsFilename = "";
	const char* m_psFilename = "";

};

#endif
