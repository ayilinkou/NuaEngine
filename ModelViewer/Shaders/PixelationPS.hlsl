Texture2D screenTexture : register(t0);
SamplerState samplerState : register(s0);

cbuffer PixelBuffer : register(b1)
{
	float2 texelSize;
	float blockSize;
	float padding;
};

struct PS_In
{
	float4 Pos : SV_POSITION;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD0;
};

float4 main(PS_In p) : SV_TARGET
{
	float2 blockTexelSize = texelSize * blockSize;
	float2 blockUV = floor(p.TexCoord / blockTexelSize) * blockTexelSize + (blockTexelSize * 0.5f);
	return screenTexture.Sample(samplerState, blockUV);
}