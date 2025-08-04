Texture2D ScreenTexture : register(t0);
Texture2D LastComposedTexture : register(t1);

SamplerState samplerState : register(s0);

cbuffer TAABuffer : register(b1)
{
    float Alpha;
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
    float4 CurrentPixel = ScreenTexture.Sample(samplerState, p.TexCoord);
    float4 LastComposedPixel = LastComposedTexture.Sample(samplerState, p.TexCoord);
    
    return lerp(CurrentPixel, LastComposedPixel, Alpha);
}