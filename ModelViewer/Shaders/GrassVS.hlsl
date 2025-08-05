#include "Common.hlsl"
#include "GlobalCBuffer.hlsl"

Texture2D Heightmap : register(t0);
StructuredBuffer<GrassData> Grass : register(t1);

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

cbuffer WindBuffer : register(b2)
{
	float Freq;
	float Amp;
	float2 WindDir;
	float TimeScale;
	float FreqMultiplier;
	float AmpMultiplier;
	uint WaveCount;
	float WindStrength;
	float SwayExponent;
	float2 MorePadding;
}

struct VS_In
{
	float2 Pos : POSITION;
	uint InstanceID : SV_InstanceID;
};

struct VS_Out
{
	float4 Pos : SV_POSITION;
	float3 WorldPos : WORLDPOS;
	float2 UV : TEXCOORD0;
	uint ChunkID : TEXCOORD1;
	float HeightAlongBlade : TEXCOORD2;
	uint LOD : TEXCOORD3;
    float4 PrevClipPos : TEXCOORD4;
};

void Animate(const in VS_Out o, float2 GrassPos, float Time, inout float3 WorldPos)
{
	float2 Noise = PerlinNoise2D(GrassPos * 0.1f) * 8.f;
    float Input = dot(GrassPos + Noise, WindDir) - Time * TimeScale;
    float2 WindOffset = SumOfSines(Input, WindDir, FreqMultiplier, AmpMultiplier, WaveCount, Hash((float)o.ChunkID));
    WindOffset += PerlinNoise2D(GrassPos * Freq) * Amp;
    WindOffset *= pow(o.HeightAlongBlade, SwayExponent) * WindDir * WindStrength;
    WorldPos.xz += WindOffset;
	
    float WindAmount = length(WindOffset);
    float BendFactor = 0.3f;
    WorldPos.y -= min(WindAmount * BendFactor, 0.8f);
}

VS_Out main(VS_In v)
{
	VS_Out o;
	float2 GrassPos = Grass[v.InstanceID].Offset;
	
	o.ChunkID = Grass[v.InstanceID].ChunkID;
	o.LOD = Grass[v.InstanceID].LOD;
	
	// apply random rotation
	float3 RotatedPos = Rotate(float3(v.Pos.xy, 0.f), float3(0.f, 1.f, 0.f), RandomAngle(GrassPos));
	
	// apply height offset
	o.UV = GetHeightmapUV(GrassPos, PlaneDimension);
	float NoiseVal = Heightmap.SampleLevel(LinearSampler, o.UV, 0.f).r;
	float Height = NoiseVal * HeightDisplacement;
	
	RotatedPos.y *= lerp(0.1f, 2.f, NoiseVal);
	o.HeightAlongBlade = RotatedPos.y;
	
	o.WorldPos = RotatedPos * 2.f + float3(GrassPos.x, Height, GrassPos.y);
    float3 PrevWorldPos = o.WorldPos;
	
	// apply wind if not root vertex
	if (v.Pos.y != 0.f)
	{		
        Animate(o, GrassPos, GlobalBuffer.CurrTime, o.WorldPos);
        Animate(o, GrassPos, GlobalBuffer.PrevTime, PrevWorldPos);
    }
	
	o.Pos = mul(float4(o.WorldPos, 1.f), GlobalBuffer.CurrViewProjJittered);
    o.PrevClipPos = mul(float4(PrevWorldPos, 1.f), GlobalBuffer.PrevViewProj);
	
	return o;
}
