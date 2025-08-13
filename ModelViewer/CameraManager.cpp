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

	m_InverseProj = DirectX::XMMatrixInverse(nullptr, m_CurrJitteredProjMatrix);
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
		// set jittered matrices as non-jittered;
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

void CameraManager::ExtractFrustumPlanes(DirectX::XMFLOAT4 FrustumPlanes[6])
{
	DirectX::XMFLOAT4X4 m;
	DirectX::XMStoreFloat4x4(&m, m_MainCamera->GetViewProjMatrix());

	// Left plane
	FrustumPlanes[0].x = m._14 + m._11;
	FrustumPlanes[0].y = m._24 + m._21;
	FrustumPlanes[0].z = m._34 + m._31;
	FrustumPlanes[0].w = m._44 + m._41;

	// Right plane
	FrustumPlanes[1].x = m._14 - m._11;
	FrustumPlanes[1].y = m._24 - m._21;
	FrustumPlanes[1].z = m._34 - m._31;
	FrustumPlanes[1].w = m._44 - m._41;

	// Bottom plane
	FrustumPlanes[2].x = m._14 + m._12;
	FrustumPlanes[2].y = m._24 + m._22;
	FrustumPlanes[2].z = m._34 + m._32;
	FrustumPlanes[2].w = m._44 + m._42;

	// Top plane
	FrustumPlanes[3].x = m._14 - m._12;
	FrustumPlanes[3].y = m._24 - m._22;
	FrustumPlanes[3].z = m._34 - m._32;
	FrustumPlanes[3].w = m._44 - m._42;

	// Near plane
	FrustumPlanes[4].x = m._13;
	FrustumPlanes[4].y = m._23;
	FrustumPlanes[4].z = m._33;
	FrustumPlanes[4].w = m._43;

	// Far plane
	FrustumPlanes[5].x = m._14 - m._13;
	FrustumPlanes[5].y = m._24 - m._23;
	FrustumPlanes[5].z = m._34 - m._33;
	FrustumPlanes[5].w = m._44 - m._43;

	// Normalise all frustum planes
	for (int i = 0; i < 6; i++)
	{
		DirectX::XMVECTOR n = DirectX::XMLoadFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(&FrustumPlanes[i]));
		float Length = DirectX::XMVectorGetX(DirectX::XMVector3Length(n));
		FrustumPlanes[i].x /= Length;
		FrustumPlanes[i].y /= Length;
		FrustumPlanes[i].z /= Length;
		FrustumPlanes[i].w /= Length;
	}
}