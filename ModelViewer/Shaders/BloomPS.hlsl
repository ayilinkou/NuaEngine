Texture2D screenTexture : register(t0);
Texture2D blurredTexture : register(t1);
SamplerState samplerState : register(s0);

cbuffer BloomBuffer : register(b1)
{
	float luminanceThreshold;
	float3 padding;
};

struct PS_In
{
	float4 Pos : SV_POSITION;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD0;
};

float CalcLuminance(float3 color)
{
	return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

float4 LuminancePS(PS_In p) : SV_TARGET
{
	float4 color = screenTexture.Sample(samplerState, p.TexCoord);
	if (CalcLuminance(color.xyz) < luminanceThreshold)
	{
		return float4(0.f, 0.f, 0.f, 1.f);
	}
	
	return float4(color.xyz, 1.f);
}

float4 BloomPS(PS_In p) : SV_TARGET
{
	return float4((screenTexture.Sample(samplerState, p.TexCoord) + blurredTexture.Sample(samplerState, p.TexCoord)).xyz, 1.f);
}