#pragma once

#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include <memory>
#include <vector>

#include "DirectXMath.h"

#include "Delegate.h"

class Camera;

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

	std::shared_ptr<Camera>& CreateCamera(DirectX::XMMATRIX ProjMatrix, bool bSetActiveCamera = true);
	void SetActiveCamera(int ID);
	void SetMainCamera(std::shared_ptr<Camera>& Camera) { m_MainCamera = Camera; }

	std::shared_ptr<Camera> GetActiveCamera() const { return m_ActiveCamera; }
	std::shared_ptr<Camera> GetMainCamera() const { return m_MainCamera; }
	std::vector<std::shared_ptr<Camera>>& GetCameras() { return m_Cameras; }

	int GetActiveCameraID() { return m_ActiveCameraID; }
	const CameraSettings& GetCameraSettings() const { return m_CameraSettings; }
	void SetCameraSpeed(float NewCameraSpeed) { m_CameraSettings.CameraSpeed = NewCameraSpeed; }

	void BindOnActiveCameraChanged(const std::function<void()>& Callback) { m_OnActiveCameraChanged.Bind(Callback); }

private:
	std::shared_ptr<Camera> m_ActiveCamera;
	std::shared_ptr<Camera> m_MainCamera;
	std::vector<std::shared_ptr<Camera>> m_Cameras;

	CameraSettings m_CameraSettings;
	int m_ActiveCameraID;

	Delegate<void()> m_OnActiveCameraChanged;
};

#endif
