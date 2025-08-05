#include "Common.hlsl"

Texture2D diffuseTexture : register(t0);
Texture2D specularTexture : register(t1);

cbuffer Material : register(b1)
{
	float3 DiffuseColor;
	int DiffuseSRV;
	float Specular;
	int SpecularSRV;
	float2 Padding;
};

struct PS_In
{
	float4 Pos : SV_POSITION;
	float3 WorldPos : POSITION;
	float2 TexCoord : TEXCOORD0;
	float3 Normal : NORMAL;
};

float4 main(PS_In p) : SV_TARGET
{		
	if (DiffuseSRV >= 0)
	{
		return diffuseTexture.Sample(LinearSampler, p.TexCoord);
	}
	return float4(DiffuseColor, 1.f);
}