#include "Common.hlsl"

Texture2D diffuseTexture : register(t0);
Texture2D specularTexture : register(t1);

struct MaterialData
{
    float3 DiffuseColor;
    int DiffuseSRV;
    float Specular;
    int SpecularSRV;
    float2 Padding;
};

cbuffer Material : register(b1)
{
    MaterialData Mat;
};

struct PS_In
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 WorldNormal : NORMAL0;
    float3 ViewNormal : NORMAL1;
    float4 CurrClipPos : TEXCOORD1;
    float4 PrevClipPos : TEXCOORD2;
};

struct PS_Out
{
    float4 Color : SV_TARGET0;
    float4 NormalSpec : SV_TARGET1;
    float2 Velocity : SV_TARGET2;
};

PS_Out main(PS_In p)
{
    PS_Out o;
	
    if (Mat.DiffuseSRV >= 0)
    {
        o.Color = diffuseTexture.Sample(LinearSampler, p.TexCoord);
        clip(o.Color.a < 1.f ? -1.f : 1.f);
    }
    else
    {
        o.Color = float4(Mat.DiffuseColor, 1.f);
    }
	
    o.NormalSpec.xyz = p.ViewNormal;
    
    if (Mat.SpecularSRV >= 0)
    {
        o.NormalSpec.a = specularTexture.Sample(LinearSampler, p.TexCoord).r;
    }
    else
    {
        o.NormalSpec.a = Mat.Specular;
    }
	
    o.Velocity = CalculateMotionVector(p.CurrClipPos, p.PrevClipPos);
	
    return o;
}