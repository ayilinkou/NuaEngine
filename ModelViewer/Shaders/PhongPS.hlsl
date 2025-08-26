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
    float3 WorldNormal : NORMAL0;
    float3 ViewNormal : NORMAL1;
    float4 CurrClipPos : TEXCOORD1;
    float4 PrevClipPos : TEXCOORD2;
};

struct PS_Out
{
    float4 Color : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float2 Velocity : SV_TARGET2;
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
	
	float3 PixelToCam = normalize(GlobalBuffer.Camera.ActiveCameraPos - p.WorldPos);
	float4 LightTotal = float4(0.f, 0.f, 0.f, 0.f);
	
	if (dot(GlobalBuffer.Camera.ActiveCameraPos, p.WorldNormal) < 0.f) // checking if surface we are looking at is on the opposite side of the normal vector and flipping if that's the case
		p.WorldNormal = -p.WorldNormal;
	
    LightTotal += CalcDirectionalLights(Color.rgb, p.WorldPos, p.WorldNormal, PixelToCam, 1.f);
    LightTotal += CalcPointLights(Color.rgb, p.WorldPos, p.WorldNormal, PixelToCam, 1.f);
	
	o.Color = Ambient + LightTotal;
    o.Normal = float4(p.ViewNormal, 0.f);
    o.Velocity = CalculateMotionVector(p.CurrClipPos, p.PrevClipPos);
	
    return o;
}

float4 mainTransparent(PS_In p) : SV_TARGET
{	
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
	
    float3 PixelToCam = normalize(GlobalBuffer.Camera.ActiveCameraPos - p.WorldPos);
    float4 LightTotal = float4(0.f, 0.f, 0.f, 0.f);
	
    if (dot(GlobalBuffer.Camera.ActiveCameraPos, p.WorldNormal) < 0.f) // checking if surface we are looking at is on the opposite side of the normal vector and flipping if that's the case
        p.WorldNormal = -p.WorldNormal;
	
    LightTotal += CalcDirectionalLights(Color.rgb, p.WorldPos, p.WorldNormal, PixelToCam, 1.f);
    LightTotal += CalcPointLights(Color.rgb, p.WorldPos, p.WorldNormal, PixelToCam, 1.f);
		
    return Ambient + LightTotal;
}