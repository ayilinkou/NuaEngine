#include "Common.hlsl"
#include "GlobalCBuffer.hlsl"

Texture2D ScreenTexture : register(t0);
Texture2D DepthTexture : register(t1);
Texture2D NormalTexture : register(t2);

cbuffer SSAOBuffer : register(b1)
{
    float Radius;
    float3 Padding;
};

struct PS_In
{
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PS_In p) : SV_TARGET
{
    float3 Color = ScreenTexture.Sample(LinearSampler, p.TexCoord).rgb;
    float Depth = DepthTexture.Sample(PointSampler, p.TexCoord).r;
    float3 Normal = NormalTexture.Sample(PointSampler, p.TexCoord).xyz;
    
    return float4(Color, 1.f);
}