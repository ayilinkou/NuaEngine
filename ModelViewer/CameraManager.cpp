#include "CameraManager.h"
#include "Camera.h"
#include "PostProcess/PostProcessManager.h"

CameraManager::CameraManager() : m_ActiveCameraID(0), m_NumJitterSamples(8)
{
}

CameraManager::~CameraManager()
{
	m_PostProcessManager.reset();
}

void CameraManager::Tick(uint32_t FrameIndex, const std::pair<int, int>& Dimensions)
{
	for (const std::shared_ptr<Camera>& c : m_Cameras)
	{
		c->CalcViewMatrix();
	}

	m_PrevJitteredProjMatrix = m_CurrJitteredProjMatrix;
	CalcJitteredMatrices(FrameIndex, Dimensions);
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

float CameraManager::Halton(int index, int base) const
{
	float f = 1.f;
	float result = 0.f;
	while (index > 0)
	{
		f /= base;
		result += f * (index % base);
		index = index / base;
	}
	return result;
}

void CameraManager::CalcJitteredMatrices(uint32_t FrameIndex, const std::pair<int, int>& Dimensions)
{
	if (!m_PostProcessManager->IsUsingTAA())
	{
		m_ActiveCamera->GetProjMatrix(m_CurrJitteredProjMatrix);
		m_ActiveCamera->GetViewProjMatrix(m_CurrJitteredViewProjMatrix);
		return;
	}
	
	float JitterX = Halton(FrameIndex % m_NumJitterSamples, 2) - 0.5f;
	float JitterY = Halton(FrameIndex % m_NumJitterSamples, 3) - 0.5f;

	JitterX *= (2.f / (float)Dimensions.first);
	JitterY *= (2.f / (float)Dimensions.second);

	DirectX::XMFLOAT4X4 TempMatrix;
	DirectX::XMStoreFloat4x4(&TempMatrix, m_ActiveCamera->GetProjMatrix());

	TempMatrix._31 += JitterX;
	TempMatrix._32 += JitterY;

	m_CurrJitteredProjMatrix = DirectX::XMLoadFloat4x4(&TempMatrix);
	m_CurrJitteredViewProjMatrix = m_ActiveCamera->GetViewMatrix() * m_CurrJitteredProjMatrix;
}
