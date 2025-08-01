#include <algorithm>
#include <cmath>
#include <format>

#include "ImGui/imgui.h"

#include "Camera.h"
#include "Application.h"
#include "Graphics.h"
#include "PostProcessManager.h"

Camera::Camera(const DirectX::XMMATRIX& Proj) : m_ProjMatrix(Proj)
{
	SetRotation(0.f, 0.f, 0.f);
	m_LookDir = { 0.f, 0.f, 1.f };
	m_bVisualiseFrustum = false;
	SetName(std::format("Camera_{}", GetUID()));
}

void Camera::RenderControls()
{
	ImGui::Text("Transform");

	ImGui::DragFloat3("Position", reinterpret_cast<float*>(&m_Transform.Position), 0.1f);
	ImGui::DragFloat2("Rotation", reinterpret_cast<float*>(&m_Transform.Rotation), 0.1f);

	ImGui::Dummy(ImVec2(0.f, 10.f));

	ImGui::Checkbox("Visualise View Frustum?", &m_bVisualiseFrustum);
}

void Camera::SetRotation(float x, float y, float z)
{
	x = std::clamp(x, -89.9f, 89.9f);
	y = std::fmod(y, 360.f);
	z = 0.f;

	m_Transform.Rotation = { x, y, z };
}

void Camera::SetLookDir(float x, float y, float z)
{
	m_LookDir = { x, y, z };
	DirectX::XMVECTOR v = DirectX::XMLoadFloat3(&m_LookDir);
	v = DirectX::XMVector3Normalize(v);
	DirectX::XMStoreFloat3(&m_LookDir, v);
}

void Camera::CalcViewMatrix()
{
	DirectX::XMFLOAT3 Up, ReversePosition;
	DirectX::XMVECTOR UpVector, PositionVector, LookAtVector, ReversePositionVector;
	DirectX::XMMATRIX RotationMatrix;
	float Yaw, Pitch, Roll;

	Up.x = 0.f;
	Up.y = 1.f;
	Up.z = 0.f;

	UpVector = DirectX::XMLoadFloat3(&Up);

	PositionVector = DirectX::XMLoadFloat3(&m_Transform.Position);

	ReversePosition.x = -m_Transform.Position.x;
	ReversePosition.y = -m_Transform.Position.y;
	ReversePosition.z = -m_Transform.Position.z;

	ReversePositionVector = DirectX::XMLoadFloat3(&ReversePosition);

	LookAtVector = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&m_LookDir));

	Pitch = DirectX::XMConvertToRadians(m_Transform.Rotation.x);
	Yaw = DirectX::XMConvertToRadians(m_Transform.Rotation.y);
	Roll = DirectX::XMConvertToRadians(0.f);

	RotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(Pitch, Yaw, Roll);

	LookAtVector = DirectX::XMVector3TransformCoord(LookAtVector, RotationMatrix);
	UpVector = DirectX::XMVector3TransformCoord(UpVector, RotationMatrix);

	LookAtVector = DirectX::XMVectorAdd(PositionVector, LookAtVector);

	m_ViewMatrix = DirectX::XMMatrixLookAtLH(PositionVector, LookAtVector, UpVector);
}

DirectX::XMFLOAT3 Camera::GetRotatedLookDir() const
{
	DirectX::XMVECTOR LookVector = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&m_LookDir));
	DirectX::XMMATRIX RotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(DirectX::XMConvertToRadians(m_Transform.Rotation.x), DirectX::XMConvertToRadians(m_Transform.Rotation.y), 0.f);
	
	LookVector = DirectX::XMVector3TransformCoord(LookVector, RotationMatrix);
	LookVector = DirectX::XMVector3Normalize(LookVector);

	DirectX::XMFLOAT3 RotatedDir;
	DirectX::XMStoreFloat3(&RotatedDir, LookVector);
	return RotatedDir;
}

DirectX::XMFLOAT3 Camera::GetRotatedLookRight() const
{
	DirectX::XMVECTOR LookVector = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&m_LookDir));
	DirectX::XMMATRIX RotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(DirectX::XMConvertToRadians(m_Transform.Rotation.x), DirectX::XMConvertToRadians(m_Transform.Rotation.y), 0.f);

	LookVector = DirectX::XMVector3TransformCoord(LookVector, RotationMatrix);
	LookVector = DirectX::XMVector3Normalize(LookVector);

	DirectX::XMFLOAT3 Up = { 0.f, 1.f, 0.f };
	DirectX::XMVECTOR UpVector = DirectX::XMLoadFloat3(&Up);
	DirectX::XMVECTOR RightVector = DirectX::XMVector3Cross(LookVector, UpVector);

	DirectX::XMFLOAT3 RotatedLookRight;
	DirectX::XMStoreFloat3(&RotatedLookRight, RightVector);
	return RotatedLookRight;
}
