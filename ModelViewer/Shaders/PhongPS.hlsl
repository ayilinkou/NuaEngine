#include "Common.hlsl"
#include "GlobalCBuffer.hlsl"

Texture2D diffuseTexture : register(t0);
Texture2D specularTexture : register(t1);

struct MaterialData
{
	float3 DiffuseColor;
	int DiffuseSRV;
	float3 Specular;
	int SpecularSRV;
};

cbuffer Material : register(b1)
{
	MaterialData Mat;
};

struct PS_In
{
	float4 Pos : SV_POSITION;
	float3 WorldPos : POSITION;
	float2 TexCoord : TEXCOORD0;
	float3 WorldNormal : NORMAL;
    float4 CurrClipPos : TEXCOORD1;
    float4 PrevClipPos : TEXCOORD2;
};

struct PS_Out
{
    float4 Color : SV_TARGET0;
    float2 Velocity : SV_TARGET1;
};

PS_Out main(PS_In p)
{		
    PS_Out o;
	
	float4 Color;
	if (Mat.DiffuseSRV >= 0)
	{
		Color = diffuseTexture.Sample(LinearSampler, p.TexCoord);
	}
	else
	{
		Color = float4(Mat.DiffuseColor, 1.f);
	}
	
	clip(Color.a < 0.1f ? -1.f : 1.f); // play around with this number
	
	float BaseAlpha = Color.a;
	float AmbientFactor = 0.5f;
	float4 Ambient = float4((Color.rgb * GlobalBuffer.Lights.SkylightColor), BaseAlpha) * AmbientFactor;
	
	float3 PixelToCam = normalize(GlobalBuffer.Camera.CameraPos - p.WorldPos);
	float4 LightTotal = float4(0.f, 0.f, 0.f, 0.f);
	
	if (dot(GlobalBuffer.Camera.CameraPos, p.WorldNormal) < 0.f) // checking if surface we are looking at is on the opposite side of the normal vector and flipping if that's the case
		p.WorldNormal = -p.WorldNormal;
	
	for (int i = 0; i < GlobalBuffer.Lights.DirectionalLightCount; i++)
	{
		float DiffuseFactor = saturate(dot(-GlobalBuffer.Lights.DirLights[i].LightDir, p.WorldNormal));
		if (DiffuseFactor <= 0.f)
			continue;
			
		float4 Diffuse = float4(GlobalBuffer.Lights.DirLights[i].LightColor, 1.f) * float4(Color.xyz, 0.5f) * DiffuseFactor;

		float3 HalfwayVec = normalize(PixelToCam + GlobalBuffer.Lights.DirLights[i].LightDir);
		float SpecularFactor = pow(saturate(dot(p.WorldNormal, HalfwayVec)), GlobalBuffer.Lights.DirLights[i].SpecularPower);
		float4 Specular = float4(GlobalBuffer.Lights.DirLights[i].LightColor, 1.f) * SpecularFactor;
		
		LightTotal += Diffuse;
		LightTotal += Specular;
	}
	
	for (int i = 0; i < GlobalBuffer.Lights.PointLightCount; i++)
	{
		float Distance = distance(p.WorldPos, GlobalBuffer.Lights.PointLights[i].LightPos);
		if (Distance > GlobalBuffer.Lights.PointLights[i].Radius)
			continue;
	
		float3 PixelToLight = normalize(GlobalBuffer.Lights.PointLights[i].LightPos - p.WorldPos);
		float DiffuseFactor = saturate(dot(PixelToLight, p.WorldNormal));
	
		if (DiffuseFactor <= 0.f)
			continue;

		float4 Diffuse = float4(GlobalBuffer.Lights.PointLights[i].LightColor, 1.f) * float4(Color.xyz, 0.5f) * DiffuseFactor;
		
		float3 HalfwayVec = normalize(PixelToCam + PixelToLight);
		float SpecularFactor = pow(saturate(dot(p.WorldNormal, HalfwayVec)), GlobalBuffer.Lights.PointLights[i].SpecularPower);
		float4 Specular = float4(GlobalBuffer.Lights.PointLights[i].LightColor, 1.f) * SpecularFactor;
	
		float Attenuation = saturate(1.f - (Distance * Distance) / (GlobalBuffer.Lights.PointLights[i].Radius * GlobalBuffer.Lights.PointLights[i].Radius)); // less control than constant, linear and quadratic, but guaranteed to reach 0 past max radius
		LightTotal += Diffuse * Attenuation;
		LightTotal += Specular * Attenuation;
	}
	
	o.Color = saturate(Ambient + LightTotal);
    o.Velocity = CalculateMotionVector(p.CurrClipPos, p.PrevClipPos);
	
    return o;
}