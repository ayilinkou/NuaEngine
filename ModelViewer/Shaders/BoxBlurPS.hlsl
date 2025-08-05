#include "Common.hlsl"

Texture2D screenTexture : register(t0);

cbuffer BlurBuffer : register(b1)
{
	float2 texelSize;
	float blurStrength;
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
	float4 colorSum = float4(0.f, 0.f, 0.f, 1.f);
	int sampleCount = 0;
	
	int effectiveBlurStrength = 25; // hard coded for now
	
	for (int i = -effectiveBlurStrength; i <= effectiveBlurStrength; i++)
	{
		float u = p.TexCoord.x + (texelSize.x * i);
		if (u < 0.f || u > 1.f)
		{
			continue;
		}
		
		colorSum += screenTexture.Sample(LinearSampler, float2(u, p.TexCoord.y));
		sampleCount++;
	}
	
	float4 color = colorSum / (float) sampleCount;
	return float4(color.xyz, 1.f);
}

float4 VerticalPS(PS_In p) : SV_TARGET
{
	float4 colorSum = float4(0.f, 0.f, 0.f, 1.f);
	int sampleCount = 0;
	
	int effectiveBlurStrength = 25; // hard coded for now
	
	for (int i = -effectiveBlurStrength; i <= effectiveBlurStrength; i++)
	{
		float v = p.TexCoord.y + (texelSize.y * i);
		if (v < 0.f || v > 1.f)
		{
			continue;
		}
		
		colorSum += screenTexture.Sample(LinearSampler, float2(p.TexCoord.x, v));
		sampleCount++;
	}
	
	float4 color = colorSum / (float) sampleCount;
	return float4(color.xyz, 1.f);
}