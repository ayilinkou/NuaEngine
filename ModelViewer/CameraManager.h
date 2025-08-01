#pragma once

#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include <memory>
#include <vector>

#include "DirectXMath.h"

#include "Delegate.h"

class Camera;
class PostProcessManager;

struct CameraSettings
{
	float CameraSpeed = 20.f;
	float CameraSpeedMin = 1.25f;
	float CameraSpeedMax = 200.f;
};

class CameraManager
{
public:
	CameraManager();
	~CameraManager();

	void SetPostProcessManager(const std::shared_ptr<PostProcessManager> PPManager) { m_PostProcessManager = PPManager; }

	void Tick(uint32_t FrameIndex, const std::pair<int, int>& Dimensions);

	std::shared_ptr<Camera>& CreateCamera(DirectX::XMMATRIX ProjMatrix, bool bSetActiveCamera = true);
	void SetActiveCamera(int ID);
	void SetMainCamera(std::shared_ptr<Camera>& Camera) { m_MainCamera = Camera; }

	std::shared_ptr<Camera> GetActiveCamera() const { return m_ActiveCamera; }
	std::shared_ptr<Camera> GetMainCamera() const { return m_MainCamera; }
	std::vector<std::shared_ptr<Camera>>& GetCameras() { return m_Cameras; }

	int GetActiveCameraID() { return m_ActiveCameraID; }
	const CameraSettings& GetCameraSettings() const { return m_CameraSettings; }
	void SetCameraSpeed(float NewCameraSpeed) { m_CameraSettings.CameraSpeed = NewCameraSpeed; }

	// if not using TAA, returns active camera's regular proj matrix
	void GetCurrJitteredProjMatrix(DirectX::XMMATRIX& CurrJitteredProjMatrix) { CurrJitteredProjMatrix = m_CurrJitteredProjMatrix; }
	// if not using TAA, returns active camera's regular proj matrix
	void GetPrevJitteredProjMatrix(DirectX::XMMATRIX& PrevJitteredProjMatrix) { PrevJitteredProjMatrix = m_PrevJitteredProjMatrix; }
	// if not using TAA, returns active camera's regular proj matrix
	DirectX::XMMATRIX GetCurrJitteredProjMatrix() const { return m_CurrJitteredProjMatrix; }
	// if not using TAA, returns active camera's regular proj matrix
	DirectX::XMMATRIX GetPrevJitteredProjMatrix() const { return m_PrevJitteredProjMatrix; }

	// if not using TAA, returns active camera's regular viewProj matrix
	void GetCurrJitteredViewProjMatrix(DirectX::XMMATRIX& CurrJitteredViewProjMatrix) { CurrJitteredViewProjMatrix = m_CurrJitteredViewProjMatrix; }
	// if not using TAA, returns active camera's regular viewProj matrix
	void GetPrevJitteredViewProjMatrix(DirectX::XMMATRIX& PrevJitteredViewProjMatrix) { PrevJitteredViewProjMatrix = m_PrevJitteredViewProjMatrix; }
	// if not using TAA, returns active camera's regular viewProj matrix
	DirectX::XMMATRIX GetCurrJitteredViewProjMatrix() const { return m_CurrJitteredViewProjMatrix; }
	// if not using TAA, returns active camera's regular viewProj matrix
	DirectX::XMMATRIX GetPrevJitteredViewProjMatrix() const { return m_PrevJitteredViewProjMatrix; }

	void BindOnActiveCameraChanged(const std::function<void()>& Callback) { m_OnActiveCameraChanged.Bind(Callback); }

private:
	float Halton(int index, int base) const;
	void CalcJitteredMatrices(uint32_t FrameIndex, const std::pair<int, int>& Dimensions);

private:
	std::shared_ptr<Camera> m_ActiveCamera;
	std::shared_ptr<Camera> m_MainCamera;
	std::vector<std::shared_ptr<Camera>> m_Cameras;
	std::shared_ptr<PostProcessManager> m_PostProcessManager;

	// these are only calculated for the active camera
	DirectX::XMMATRIX m_CurrJitteredProjMatrix;
	DirectX::XMMATRIX m_PrevJitteredProjMatrix;
	DirectX::XMMATRIX m_CurrJitteredViewProjMatrix;
	DirectX::XMMATRIX m_PrevJitteredViewProjMatrix;

	CameraSettings m_CameraSettings;
	int m_ActiveCameraID;

	Delegate<void()> m_OnActiveCameraChanged;
};

#endif
