#include "Light.h"
#include "ImGui/imgui.h"

Light::Light()
{
	m_DiffuseColor = { 1.f, 1.f, 1.f };
	m_SpecularPower = 1024.f;
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
	ImGui::ColorEdit3("Diffuse Color", reinterpret_cast<float*>(&m_DiffuseColor));
	ImGui::SliderFloat("Specular Power", &m_SpecularPower, 0.f, 2048.f, "%.f");
}

void Light::SetDiffuseColor(float r, float g, float b)
{
	m_DiffuseColor = DirectX::XMFLOAT3(r, g, b);
}

void Light::SetSpecularPower(float Power)
{
	m_SpecularPower = Power;
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

void PointLight::SetRadius(float Radius)
{
	m_Radius = Radius;
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

void DirectionalLight::SetDirection(float x, float y, float z)
{
	DirectX::XMFLOAT3 Dir = { x, y, z };
	DirectX::XMVECTOR v = DirectX::XMLoadFloat3(&Dir);
	v = DirectX::XMVector3Normalize(v);
	DirectX::XMStoreFloat3(&m_Dir, v);
}
