#include "Common.hlsl"

Texture2D screenTexture : register(t0);

cbuffer ToneMapperBuffer : register(b1)
{
	float gamma;
	float3 padding;
};

struct PS_In
{
	float4 Pos : SV_POSITION;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD0;
};

float4 main(PS_In p) : SV_TARGET
{
	float4 color = screenTexture.Sample(LinearSampler, p.TexCoord);
	
	return float4(pow(color.xyz, 1.f / gamma), 1.f);
}
