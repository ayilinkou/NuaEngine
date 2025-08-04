#include "Common.hlsl"
#include "GlobalCBuffer.hlsl"

StructuredBuffer<float4> Corners : register(t0);

cbuffer CameraBuffer : register(b1)
{
	CameraInfo Camera;
}

struct VS_IN
{
	float4 Pos : POSITION;
	uint VertexID : SV_VertexID;
	uint InstanceID : SV_InstanceID;
};

float4 main(VS_IN v) : SV_POSITION
{
    return mul(Corners[v.InstanceID * 8 + v.VertexID], GlobalBuffer.CurrViewProj);
}