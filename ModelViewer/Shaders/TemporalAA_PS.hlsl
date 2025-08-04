#include "Common.hlsl"

Texture2D ScreenTexture : register(t0);
Texture2D HistoryTexture : register(t1);
Texture2D VelocityTexture : register(t2);

SamplerState samplerState : register(s0);

cbuffer TAABuffer : register(b1)
{
    float Alpha;
    int bUseMotionVectors;
    float2 Padding;
};

struct PS_In
{
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PS_In p) : SV_TARGET
{
    float2 MotionUV = VelocityTexture.Sample(samplerState, p.TexCoord); // TODO: point sampler should be used here
    float2 ReprojectedUV = bUseMotionVectors != 0 ? p.TexCoord - MotionUV : p.TexCoord;
    
    if (IsInRange(ReprojectedUV, 0.f, 1.f))
    {
        float4 CurrentPixel = ScreenTexture.Sample(samplerState, p.TexCoord);
        float4 HistoryPixel = HistoryTexture.Sample(samplerState, ReprojectedUV);
        
        // TODO: color clamping
        
        return lerp(CurrentPixel, HistoryPixel, Alpha);
    }
    
    return ScreenTexture.Sample(samplerState, p.TexCoord);
}