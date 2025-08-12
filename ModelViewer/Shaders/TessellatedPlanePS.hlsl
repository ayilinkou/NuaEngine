#include "Common.hlsl"
#include "GlobalCBuffer.hlsl"

Texture2D Heightmap : register(t0);

cbuffer PlaneInfoBuffer : register(b1)
{
	float PlaneDimension;
	float HeightDisplacement;
	uint ChunkInstanceCount;
	bool bVisualiseChunks;
	float4x4 ChunkScaleMatrix;
	uint GrassPerChunk;
	float3 Padding;
};

struct PS_In
{
	float4 Pos : SV_POSITION;
	float3 WorldPos : WORLDPOS;
    float3 WorldNormal : NORMAL0;
    float3 ViewNormal : NORMAL1;
	float2 UV : TEXCOORD0;
	uint ChunkID : TEXCOORD1;
    float4 CurrClipPos : TEXCOORD2;
    float4 PrevClipPos : TEXCOORD3;
};

struct PS_Out
{
    float4 Color : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float2 Velocity : SV_TARGET2;
};

float3 RandomRGB(uint seed)
{
    // Jenkins-style hash
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);

    // Generate three pseudo-random floats between 0.0 and 1.0
	float r = ((seed & 0xFF)) / 255.0;
	float g = ((seed & 0xFF00)) / 65280.0;
	float b = ((seed & 0xFF0000) >> 16) / 255.0;

	return float3(r, g, b);
}

static const float4 TopColor = float4(0.30f, 0.5f, 0.1f, 1.0f);
static const float4 BotColor = float4(0.05f, 0.2f, 0.0f, 1.0f);

PS_Out main(PS_In p)
{
    PS_Out o;
	
	if (bVisualiseChunks)
    {
		o.Color = float4(RandomRGB(p.ChunkID), 1.f);
        o.Normal = float4(p.ViewNormal, 0.f);
        o.Velocity = CalculateMotionVector(p.CurrClipPos, p.PrevClipPos);
        return o;
    }

    float4 Color;
	float Height = Heightmap.Sample(LinearSampler, p.UV).r;
	if (Height < 0.03)
	{
		Color = float4(50.f / 256.f, 123.f / 256.f, 191.f / 256.f, 1.f);
	}
	else
    {
		Color = lerp(BotColor, TopColor, Height);
    }
	
    float3 PixelToCam = normalize(GlobalBuffer.Camera.CameraPos - p.WorldPos);
    float4 LightTotal = float4(0.f, 0.f, 0.f, 0.f);
    LightTotal += CalcDirectionalLights(Color.rgb, p.WorldPos, p.WorldNormal, PixelToCam);
    LightTotal += CalcPointLights(Color.rgb, p.WorldPos, p.WorldNormal, PixelToCam);
		
    o.Color = float4(LightTotal.rgb, 1.f);
    o.Normal = float4(p.ViewNormal, 0.f);
    o.Velocity = CalculateMotionVector(p.CurrClipPos, p.PrevClipPos);
    return o;
}