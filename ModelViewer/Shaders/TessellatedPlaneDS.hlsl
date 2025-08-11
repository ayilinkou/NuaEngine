#include "Common.hlsl"
#include "GlobalCBuffer.hlsl"

Texture2D Heightmap : register(t0);

struct DS_In
{
	float3 Pos : POSITION;
	float2 UV : TEXCOORD0;
	uint ChunkID : TEXCOORD1;
};

struct DS_Out
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

struct TessFactors
{
	float EdgeTessFactor[4] : SV_TessFactor;
	float InsideTessFactor[2] : SV_InsideTessFactor;
};

#define NUM_CONTROL_POINTS 4

[domain("quad")]
DS_Out main(
	TessFactors t,
	float2 UV : SV_DomainLocation,
	const OutputPatch<DS_In, NUM_CONTROL_POINTS> Patch)
{
	DS_Out o;

	float3 p0 = Patch[0].Pos;
	float3 p1 = Patch[1].Pos;
	float3 p2 = Patch[2].Pos;
	float3 p3 = Patch[3].Pos;
	
	float3 Top = lerp(p0, p1, UV.x);
	float3 Bot = lerp(p2, p3, UV.x);
	float3 Pos = lerp(Top, Bot, UV.y);
	
	float2 DS_UV = GetHeightmapUV(Pos.xz, PlaneDimension);
		
	float2 uv0 = Patch[0].UV;
	float2 uv1 = Patch[1].UV;
	float2 uv2 = Patch[2].UV;
	float2 uv3 = Patch[3].UV;
	
	float2 TopUV = lerp(uv0, uv1, UV.x);
	float2 BotUV = lerp(uv2, uv3, UV.x);
	o.UV = lerp(TopUV, BotUV, UV.y);

	float Height = Heightmap.SampleLevel(LinearSampler, DS_UV, 0.f).r * HeightDisplacement;
	
	Pos.y = Height;
	o.WorldPos = Pos;
	
    float3 dpdu = lerp(p1 - p0, p3 - p2, UV.y);
    float3 dpdv = lerp(p2 - p0, p3 - p1, UV.x);
    o.WorldNormal = normalize(cross(dpdu, dpdv));
    o.ViewNormal = normalize(mul(float4(o.WorldNormal, 0.f), GlobalBuffer.Camera.CurrView)).xyz;
	
	o.Pos = mul(float4(Pos, 1.f), GlobalBuffer.Camera.CurrViewProjJittered);
    o.CurrClipPos = mul(float4(Pos, 1.f), GlobalBuffer.Camera.CurrViewProj);
    o.PrevClipPos = mul(float4(Pos, 1.f), GlobalBuffer.Camera.PrevViewProj);
	
	o.ChunkID = Patch[0].ChunkID;

	return o;
}
