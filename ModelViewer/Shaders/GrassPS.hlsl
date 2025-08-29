#include "Common.hlsl"
#include "GlobalCBuffer.hlsl"

struct PS_In
{
	float4 Pos : SV_POSITION;
	float3 WorldPos : WORLDPOS;
    float3 WorldNormal : NORMAL0;
    float3 ViewNormal : NORMAL1;
	float2 UV : TEXCOORD0;
	uint ChunkID : TEXCOORD1;
	float HeightAlongBlade : TEXCOORD2;
	uint LOD : TEXCOORD3;
    float4 CurrClipPos : TEXCOORD4;
    float4 PrevClipPos : TEXCOORD5;
};

struct PS_Out
{
    float4 Color : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float2 Velocity : SV_TARGET2;
};

static const float4 AOColor		= float4(0.0f,  0.2f, 0.0f, 1.f);
static const float4 RootColor	= float4(0.0f,  0.3f, 0.0f, 1.f);
static const float4 MidColor	= float4(0.4f,  0.7f, 0.1f, 1.f);
static const float4 TipColor	= float4(0.9f,  1.0f, 0.2f, 1.f);

PS_Out main(PS_In p)
{
    PS_Out o;
	
    o.Color = MidColor;
	
	float TipThreshold = 2.f;
	float MidThreshold = 1.f;
	float RootThreshold = 0.2f;
	float AOThreshold = 0.f;
	
	if (p.HeightAlongBlade > MidThreshold)
	{
		o.Color = lerp(MidColor, TipColor, Remap(p.HeightAlongBlade, MidThreshold, TipThreshold, 0.f, 1.f));
	}
	else if (p.HeightAlongBlade > RootThreshold)
	{
		o.Color = lerp(RootColor, MidColor, Remap(p.HeightAlongBlade, RootThreshold, MidThreshold, 0.f, 1.f));
	}
	else if (p.HeightAlongBlade > AOThreshold)
	{
		o.Color = lerp(AOColor, RootColor, Remap(p.HeightAlongBlade, AOThreshold, RootThreshold, 0.f, 1.f));
	}
				
    o.Normal = float4(p.ViewNormal, 0.f);
    o.Velocity = CalculateMotionVector(p.CurrClipPos, p.PrevClipPos);
	
    return o;
}
