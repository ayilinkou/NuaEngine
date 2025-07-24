#pragma once

#ifndef TESSELLATED_PLANE_H
#define TESSELLATED_PLANE_H

#include <memory>

#include "d3d11.h"
#include "DirectXMath.h"

#include "wrl.h"

#include "GameObject.h"

class CameraManager;

class TessellatedPlane : public GameObject
{
	friend class Landscape;

private:
	struct HullCBuffer
	{
		DirectX::XMFLOAT3 CameraPos;
		float TessellationScale;
	};

public:
	TessellatedPlane();
	~TessellatedPlane();

	bool Init(float TessellationScale, Landscape* pLandscape);
	void Render(const std::shared_ptr<CameraManager>& CamManager);
	void Shutdown();

	virtual void RenderControls() override;

	void SetShouldRender(bool bNewShouldRender) { m_bShouldRender = bNewShouldRender; }
	bool ShouldRender() const { return m_bShouldRender; }

private:
	bool CreateShaders();
	bool CreateBuffers();

	void UpdateBuffers(const std::shared_ptr<CameraManager>& CamManager);

private:
	ID3D11VertexShader* m_VertexShader		= nullptr;
	ID3D11HullShader* m_HullShader			= nullptr;
	ID3D11DomainShader* m_DomainShader		= nullptr;
	ID3D11GeometryShader* m_GeometryShader	= nullptr;
	ID3D11PixelShader* m_PixelShader		= nullptr;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_InputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_IndexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_VertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_HullCBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_ArgsBuffer;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_ArgsBufferUAV;

	Landscape* m_pLandscape = nullptr;
	float m_TessellationScale;
	bool m_bShouldRender;

	const char* m_vsFilename = "";
	const char* m_hsFilename = "";
	const char* m_dsFilename = "";
	const char* m_gsFilename = "";
	const char* m_psFilename = "";

};

#endif
