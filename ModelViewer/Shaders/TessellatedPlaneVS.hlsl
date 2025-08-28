#include "Common.hlsl"

Texture2D Heightmap : register(t0);
StructuredBuffer<CullTransformData> Transforms : register(t1);

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
	o.Pos = mul(float4(v.Pos, 1.f), Transforms[v.InstanceID].CurrTransform).xyz;
	o.UV = GetHeightmapUV(o.Pos.xz, PlaneDimension);
	
	float Height = Heightmap.SampleLevel(LinearSampler, o.UV, 0.f).r * HeightDisplacement;
	o.Pos.y = Height;
	
    float4 ChunkOrigin = float4(0.f, 0.f, 0.f, 1.f);
    ChunkOrigin = mul(ChunkOrigin, Transforms[v.InstanceID].CurrTransform);
    o.ChunkID = HashFloat2ToUint(ChunkOrigin.xz);
	
	return o;
}