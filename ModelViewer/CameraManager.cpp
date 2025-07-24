#include "CameraManager.h"

#include "Camera.h"

CameraManager::CameraManager() : m_ActiveCameraID(0)
{
}

CameraManager::~CameraManager()
{
	Shutdown();
}

void CameraManager::Shutdown()
{
}

std::shared_ptr<Camera>& CameraManager::CreateCamera(DirectX::XMMATRIX ProjMatrix, bool bSetActiveCamera)
{
	m_Cameras.emplace_back(std::make_shared<Camera>(ProjMatrix));
	int Index = (int)m_Cameras.size() - 1;

	if (bSetActiveCamera)
	{
		SetActiveCamera(Index);
	}

	return m_Cameras.back();
}

void CameraManager::SetActiveCamera(int ID)
{
	bool bShouldBroadcast = m_ActiveCameraID != ID;

	m_ActiveCameraID = ID;
	m_ActiveCamera = m_Cameras[ID];

	if (bShouldBroadcast)
	{
		m_OnActiveCameraChanged.Broadcast();
	}
}
