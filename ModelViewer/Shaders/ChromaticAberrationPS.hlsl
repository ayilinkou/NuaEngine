#include "Common.hlsl"

Texture2D screenTexture : register(t0);

cbuffer ChromaticAberrationBuffer : register(b1)
{
    float2 TexelSize;
    float Scale;
    float Padding;
};

struct PS_In
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PS_In p) : SV_TARGET
{
    float3 Color = float3(0.f, 0.f, 0.f);
    float2 Offset = float2(1.f, 1.f) * TexelSize;
    
    Color.r = screenTexture.Sample(LinearSampler, p.TexCoord + Offset * Scale).r;
    Color.g = screenTexture.Sample(LinearSampler, p.TexCoord).g;
    Color.b = screenTexture.Sample(LinearSampler, p.TexCoord - Offset * Scale).b;
    return float4(Color, 1.f);
}