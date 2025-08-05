#include "Common.hlsl"

Texture2D screenTexture : register(t0);

StructuredBuffer<float> gaussianWeights : register(t1);

cbuffer BlurBuffer : register(b1)
{
	float2 texelSize;
	int blurStrength;
	float padding;
};

struct PS_In
{
	float4 Pos : SV_POSITION;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD0;
};

float4 HorizontalPS(PS_In p) : SV_TARGET
{
	float3 colorSum = float3(0.f, 0.f, 0.f);
	float weightSum = 0.f;
		
	for (int i = 0; i <= blurStrength; i++)
	{
		float offset = texelSize.x * i;
		float u;
		
		u = p.TexCoord.x + offset;
		if (u >= 0.f && u <= 1.f)
		{
			colorSum += screenTexture.Sample(LinearSampler, float2(u, p.TexCoord.y)).xyz * gaussianWeights[i];
			weightSum += gaussianWeights[i];
		}
		
		if (i == 0)
		{
			continue;
		}
		
		u = p.TexCoord.x - offset;
		if (u >= 0.f && u <= 1.f)
		{
			colorSum += screenTexture.Sample(LinearSampler, float2(u, p.TexCoord.y)).xyz * gaussianWeights[i];
			weightSum += gaussianWeights[i];
		}
	}
	
	float3 color = colorSum / weightSum;
	return float4(color.xyz, 1.f);
}

float4 VerticalPS(PS_In p) : SV_TARGET
{
	float3 colorSum = float3(0.f, 0.f, 0.f);
	float weightSum = 0.f;
		
	for (int i = 0; i <= blurStrength; i++)
	{
		float offset = texelSize.y * i;
		float v;
		
		v = p.TexCoord.y + offset;
		if (v >= 0.f && v <= 1.f)
		{
			colorSum += screenTexture.Sample(LinearSampler, float2(p.TexCoord.x, v)).xyz * gaussianWeights[i];
			weightSum += gaussianWeights[i];
		}
		
		if (i == 0)
		{
			continue;
		}
		
		v = p.TexCoord.y - offset;
		if (v >= 0.f && v <= 1.f)
		{
			colorSum += screenTexture.Sample(LinearSampler, float2(p.TexCoord.x, v)).xyz * gaussianWeights[i];
			weightSum += gaussianWeights[i];
		}
	}
	
	float3 color = colorSum / weightSum;
	return float4(color.xyz, 1.f);
}