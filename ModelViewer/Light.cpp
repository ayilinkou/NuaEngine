#include "ImGui/imgui.h"

#include "Light.h"
#include "Graphics.h"

Light::Light()
{
	m_Color = { 1.f, 1.f, 1.f };
	m_SpecularPower = 512.f;
	m_Intensity = 1.f;
	m_bActive = true;

	m_ComponentName = "Light";
	m_Lights.push_back(this);
}

Light::~Light()
{
	auto it = std::find(m_Lights.begin(), m_Lights.end(), this);
	if (it != m_Lights.end())
	{
		m_Lights.erase(it);
	}
}

void Light::RenderControls()
{
	ImGui::Text(m_ComponentName.c_str());
	ImGui::Checkbox("Active", &m_bActive);
	ImGui::ColorEdit3("Diffuse Color", reinterpret_cast<float*>(&m_Color));
	ImGui::SliderFloat("Specular Power", &m_SpecularPower, 0.f, 2048.f, "%.f");
	ImGui::SliderFloat("Intensity", &m_Intensity, 0.f, 10.f, "%.1f");
}

void Light::SetDirection(float x, float y, float z)
{
	DirectX::XMFLOAT3 Dir = { x, y, z };
	DirectX::XMVECTOR v = DirectX::XMLoadFloat3(&Dir);
	v = DirectX::XMVector3Normalize(v);
	DirectX::XMStoreFloat3(&m_Dir, v);
}

//////////////////////////////////////////////////////////////

PointLight::PointLight()
{
	m_Radius = 10.f;

	m_ComponentName = "Point Light";
}

void PointLight::RenderControls()
{
	Light::RenderControls();
	ImGui::SliderFloat("Radius", &m_Radius, 0.f, 1000.f, "%.f", ImGuiSliderFlags_AlwaysClamp);
}

//////////////////////////////////////////////////////////////

DirectionalLight::DirectionalLight()
{
	SetDirection(1.f, -1.f, 1.f);

	m_ComponentName = "Directional Light";
}

void DirectionalLight::RenderControls()
{
	Light::RenderControls();
	
	float Dir[3] = { m_Dir.x, m_Dir.y, m_Dir.z };
	ImGui::SliderFloat3("Direction", Dir, -1.f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
	if (memcmp(&Dir, &m_Dir, sizeof(float) * 3) != 0)
	{
		SetDirection(Dir[0], Dir[1], Dir[2]);
	}
}

//////////////////////////////////////////////////////////////

SpotLight::SpotLight()
{
	m_Radius = 10.f;
	m_ConeInnerAngle = 45.f;
	m_ConeOuterAngle = 60.f;
	m_Dir = { 0.f, -1.f, 0.f };
	m_ComponentName = "Spot Light";
}

void SpotLight::RenderControls()
{
	Light::RenderControls();
	ImGui::SliderFloat("Radius", &m_Radius, 0.f, 1000.f, "%.f", ImGuiSliderFlags_AlwaysClamp);
	if (ImGui::SliderFloat("Cone Inner Angle", &m_ConeInnerAngle, 0.f, 180.f, "%.f", ImGuiSliderFlags_AlwaysClamp))
	{
		if (m_ConeInnerAngle > m_ConeOuterAngle)
			m_ConeOuterAngle = m_ConeInnerAngle + 0.01f;
	}

	if (ImGui::SliderFloat("Cone Outer Angle", &m_ConeOuterAngle, 0.f, 180.f, "%.f", ImGuiSliderFlags_AlwaysClamp))
	{
		if (m_ConeOuterAngle < m_ConeInnerAngle)
			m_ConeInnerAngle = m_ConeOuterAngle - 0.01f;
	}

	float Dir[3] = { m_Dir.x, m_Dir.y, m_Dir.z };
	ImGui::SliderFloat3("Direction", Dir, -1.f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
	if (memcmp(&Dir, &m_Dir, sizeof(float) * 3) != 0)
	{
		SetDirection(Dir[0], Dir[1], Dir[2]);
	}
}
