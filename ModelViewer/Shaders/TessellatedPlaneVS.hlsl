#include "Common.hlsl"

Texture2D Heightmap : register(t0);
StructuredBuffer<float2> Offsets : register(t1);

SamplerState Sampler : register(s0);

cbuffer PlaneInfoBuffer : register(b1)
{
	float PlaneDimension;
	float HeightDisplacement;
	uint ChunkInstanceCount;
	bool bVisualiseChunks;
	float4x4 ChunkScaleMatrix;
	uint GrassPerChunk;
	float Time;
	float2 Padding;
};

struct VS_In
{
	float3 Pos : POSITION;
	uint InstanceID : SV_InstanceID;
};

struct VS_Out
{
	float3 Pos : POSITION;
	float2 UV : TEXCOORD0;
	uint ChunkID : TEXCOORD1;
};

VS_Out main(VS_In v)
{
	VS_Out o;
	o.Pos = mul(float4(v.Pos, 1.f), ChunkScaleMatrix).xyz + float3(Offsets[v.InstanceID].x, 0.f, Offsets[v.InstanceID].y);
	o.UV = GetHeightmapUV(o.Pos.xz, PlaneDimension);
	
	float Height = Heightmap.SampleLevel(Sampler, o.UV, 0.f).r * HeightDisplacement;
	o.Pos.y = Height;
	
	o.ChunkID = HashFloat2ToUint(Offsets[v.InstanceID]);
	
	return o;
}