#pragma once

#ifndef LIGHT_H
#define LIGHT_H

#include <DirectXMath.h>

#include "Component.h"

#include "GameObject.h"

class Light : public Component
{
public:
	virtual ~Light();

	virtual void RenderControls() override;

	void SetColor(float r, float g, float b) { m_Color = DirectX::XMFLOAT3(r, g, b); }
	void SetColor(DirectX::XMFLOAT3 Color) { m_Color = Color; }
	void SetSpecularPower(float Power) { m_SpecularPower = Power; }
	void SetIntensity(float Intensity) { m_Intensity = Intensity; }
	void SetDirection(float x, float y, float z);

	DirectX::XMFLOAT3 GetColor() const { return m_Color; }
	DirectX::XMFLOAT3 GetDirection() const { return m_Dir; }
	float GetSpecularPower() const { return m_SpecularPower; }
	float GetIntensity() const { return m_Intensity; }
	bool IsActive() const { return m_bActive; }
	static std::vector<Light*>& GetLights() { return m_Lights; }

protected:
	Light();

protected:
	DirectX::XMFLOAT3 m_Color;
	DirectX::XMFLOAT3 m_Dir;
	float m_SpecularPower;
	float m_Intensity;
	bool m_bActive;

	inline static std::vector<Light*> m_Lights;
};

class PointLight : public Light
{
public:
	PointLight();

	virtual void RenderControls() override;

	void SetRadius(float Radius) { m_Radius = Radius; }

	DirectX::XMFLOAT3 GetPosition() const { return GetOwner()->GetPosition(); }
	float GetRadius() const { return m_Radius; }

private:
	float m_Radius;
};

class DirectionalLight : public Light
{
public:
	DirectionalLight();

	virtual void RenderControls() override;
};

class SpotLight : public Light
{
public:
	SpotLight();

	virtual void RenderControls() override;

	void SetRadius(float Radius) { m_Radius = Radius; }

	DirectX::XMFLOAT3 GetPosition() const { return GetOwner()->GetPosition(); }
	float GetRadius() const { return m_Radius; }
	float GetConeInnerAngle() const { return m_ConeInnerAngle; }
	float GetConeOuterAngle() const { return m_ConeOuterAngle; }

private:
	float m_Radius;
	float m_ConeInnerAngle;
	float m_ConeOuterAngle;
};

#endif
