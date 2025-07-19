#pragma once

#ifndef CAMERA_H
#define CAMERA_H

#include <DirectXMath.h>

#include "GameObject.h"

class Camera : public GameObject
{
public:
	Camera() = delete;
	Camera(const DirectX::XMMATRIX& Proj);

	virtual void RenderControls() override;

	virtual void SetRotation(float x, float y, float z) override;
	void SetLookDir(float, float, float);

	void CalcViewMatrix();

	DirectX::XMFLOAT3 GetLookDir() const { return m_LookDir; }
	DirectX::XMFLOAT3 GetRotatedLookDir() const;
	DirectX::XMFLOAT3 GetRotatedLookRight() const;

	void GetViewMatrix(DirectX::XMMATRIX& ViewMatrix) { ViewMatrix = m_ViewMatrix; }
	DirectX::XMMATRIX GetViewMatrix() const { return m_ViewMatrix; }
	void GetProjMatrix(DirectX::XMMATRIX& ProjMatrix);
	DirectX::XMMATRIX GetProjMatrix();
	void GetProjMatrixJitterFree(DirectX::XMMATRIX& ProjMatrix) { ProjMatrix = m_ProjMatrix; }
	DirectX::XMMATRIX GetProjMatrixJitterFree() const { return m_ProjMatrix; }
	void GetViewProjMatrix(DirectX::XMMATRIX& ViewProj) { ViewProj = m_ViewMatrix * GetProjMatrix(); }
	DirectX::XMMATRIX GetViewProjMatrix() { return m_ViewMatrix * GetProjMatrix(); }

	bool ShouldVisualiseFrustum() const { return m_bVisualiseFrustum; }

private:
	float Halton(int index, int base) const;
	void CalculateJitteredProjMatrix(uint32_t FrameIndex);

private:
	DirectX::XMFLOAT3 m_LookDir;
	DirectX::XMMATRIX m_ViewMatrix;
	DirectX::XMMATRIX m_ProjMatrix;
	DirectX::XMMATRIX m_ProjMatrixJittered;

	bool m_bVisualiseFrustum;
	uint32_t m_LastCalculatedFrame = 0;

};

#endif
