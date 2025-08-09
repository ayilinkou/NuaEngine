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

	void SetDiffuseColor(float r, float g, float b);
	void SetSpecularPower(float Power);

	const DirectX::XMFLOAT3 GetDiffuseColor() const { return m_DiffuseColor; }
	float GetSpecularPower() const { return m_SpecularPower; }
	bool IsActive() const { return m_bActive; }
	static std::vector<Light*>& GetLights() { return m_Lights; }

protected:
	Light();

private:
	DirectX::XMFLOAT3 m_DiffuseColor;
	float m_SpecularPower;
	bool m_bActive;

	inline static std::vector<Light*> m_Lights;
};

class PointLight : public Light
{
public:
	PointLight();

	virtual void RenderControls() override;

	void SetRadius(float Radius);

	DirectX::XMFLOAT3 GetPosition() const { return GetOwner()->GetPosition(); }
	float GetRadius() const { return m_Radius; }

private:
	// DirectX::XMFLOAT3 m_Position; // can add this back when adding accumulated transform
	float m_Radius;

};

class DirectionalLight : public Light
{
public:
	DirectionalLight();

	virtual void RenderControls() override;

	void SetDirection(float x, float y, float z);

	const DirectX::XMFLOAT3 GetDirection() const { return m_Dir; }

private:
	DirectX::XMFLOAT3 m_Dir;

};

#endif
