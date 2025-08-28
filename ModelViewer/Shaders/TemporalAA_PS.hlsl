#include "Common.hlsl"
#include "GlobalCBuffer.hlsl"

Texture2D ScreenTexture : register(t0);
Texture2D HistoryTexture : register(t1);
Texture2D VelocityTexture : register(t2);

cbuffer TAABuffer : register(b1)
{
    float Alpha;
    int bUseMotionVectors;
    int bUseColorClamping;
    float Padding;
};

struct PS_In
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PS_In p) : SV_TARGET
{
    float2 MotionUV = VelocityTexture.Sample(PointClampSampler, p.TexCoord);
    float2 ReprojectedUV = bUseMotionVectors != 0 ? p.TexCoord - MotionUV : p.TexCoord;
    
    if (IsInRange(ReprojectedUV, 0.f, 1.f))
    {
        float4 CurrentPixel = ScreenTexture.Sample(LinearSampler, p.TexCoord);
        float4 HistoryPixel = HistoryTexture.SampleLevel(PointClampSampler, ReprojectedUV, 0.f);

        if (bUseColorClamping)
        {
            const float MaxFloat = asfloat(0x7f7fffff);
            float3 MinColor = float3(MaxFloat, MaxFloat, MaxFloat);
            float3 MaxColor = float3(-MaxFloat, -MaxFloat, -MaxFloat);
            float2 TexelSize = float2(1.f, 1.f) / GlobalBuffer.ScreenRes;
        
            for (int dx = -1; dx <= 1; ++dx)
            {
                for (int dy = -1; dy <= 1; ++dy)
                {
                    float2 SampleUV = ReprojectedUV + float2(dx, dy) * TexelSize;
                    float3 Neighbour = ScreenTexture.SampleLevel(PointClampSampler, SampleUV, 0.f);

                    MinColor = min(MinColor, Neighbour);
                    MaxColor = max(MaxColor, Neighbour);
                }
            }

            HistoryPixel.rgb = clamp(HistoryPixel.rgb, MinColor, MaxColor);
        }
        
        return lerp(CurrentPixel, HistoryPixel, Alpha);
    }
    
    return ScreenTexture.Sample(LinearSampler, p.TexCoord);
}