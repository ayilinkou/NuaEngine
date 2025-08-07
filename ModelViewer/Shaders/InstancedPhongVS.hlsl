#include "Common.hlsl"
#include "GlobalCBuffer.hlsl"

StructuredBuffer<CullTransformData> CulledTransforms : register(t0);

cbuffer AccumulatedModelTransform : register(b1)
{
	float4x4 AccumulatedModelMatrix; // if I ever want to animate certain nodes or be able to transform them independently, I'll have to store prev and curr
}

struct VS_In
{
	float3 Pos : POSITION;
	float2 TexCoord : TEXCOORD0;
	float3 Normal : NORMAL;
	
	uint InstanceID : SV_InstanceID;
};

struct VS_Out
{
	float4 Pos : SV_POSITION;
	float3 WorldPos : POSITION;
	float2 TexCoord : TEXCOORD0;
	float3 WorldNormal : NORMAL;
    float4 CurrClipPos : TEXCOORD1;
    float4 PrevClipPos : TEXCOORD2;
};

VS_Out main(VS_In v)
{
	VS_Out o;
	
	// mesh vertices have no knowledge whether they are parented to a parent mesh node or not
	// to solve this, multiply by the AccumulatedModelMatrix BEFORE applying model transform
	o.Pos = mul(mul(float4(v.Pos, 1.f), AccumulatedModelMatrix), CulledTransforms[v.InstanceID].CurrTransform);
	
	o.WorldPos = o.Pos.xyz;
	
	o.Pos = mul(o.Pos, GlobalBuffer.CurrViewProjJittered);
	
    o.CurrClipPos = mul(float4(o.WorldPos, 1.f), GlobalBuffer.CurrViewProj);
    o.PrevClipPos = mul(mul(float4(v.Pos, 1.f), AccumulatedModelMatrix), CulledTransforms[v.InstanceID].PrevTransform);
    o.PrevClipPos = mul(o.PrevClipPos, GlobalBuffer.PrevViewProj);
	
	o.TexCoord = v.TexCoord;
	
	o.WorldNormal = mul(mul(float4(v.Normal, 0.f), AccumulatedModelMatrix), CulledTransforms[v.InstanceID].CurrTransform).xyz; // TODO: correct as long as I use uniform scaling, come back and fix
	o.WorldNormal = normalize(o.WorldNormal);
	
	return o;
}