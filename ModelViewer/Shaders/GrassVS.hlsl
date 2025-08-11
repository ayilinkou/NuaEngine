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
    uint VertexID : SV_VertexID;
	uint InstanceID : SV_InstanceID;
};

struct VS_Out
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

static const float2 GrassVertices[11] =
{
    { -0.028f, 0.0f },
    {  0.028f, 0.0f },
    { -0.026f, 0.2f },
    {  0.026f, 0.2f },
    { -0.024f, 0.4f },
    {  0.024f, 0.4f },
    { -0.022f, 0.6f },
    {  0.022f, 0.6f },
    { -0.020f, 0.8f },
    {  0.020f, 0.8f },
    {  0.000f, 1.0f }
};

static const float2 GrassVerticesLOD[3] =
{
    {{ -0.058f, 0.0f }},
    {{  0.058f, 0.0f }},
    {{  0.000f, 1.0f }},
};

void Animate(const in VS_Out o, float2 GrassPos, float Time, inout float3 WorldPos)
{
    // apply wind if not root vertex
    if (o.HeightAlongBlade != 0.f)
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
}

VS_Out CalcWorldPos(const in VS_In v, uint VertexID)
{
    VS_Out o;
    
    float2 GrassPos = Grass[v.InstanceID].Offset;
    o.ChunkID = Grass[v.InstanceID].ChunkID;
    o.LOD = Grass[v.InstanceID].LOD;
    float3 Pos = o.LOD == 0u ? float3(GrassVertices[VertexID], 0.f) : float3(GrassVerticesLOD[VertexID], 0.f);
	
	// apply random rotation
    const float3 Normal = float3(0.f, 0.f, 1.f);
    const float3 Up = float3(0.f, 1.f, 0.f);
    const float RotationAngle = RandomAngle(GrassPos);
    float3 RotatedPos = Rotate(Pos, Up, RotationAngle);
    o.WorldNormal = normalize(Rotate(Normal, Up, RotationAngle));
    o.ViewNormal = normalize(mul(float4(o.WorldNormal, 0.f), GlobalBuffer.Camera.CurrView).xyz);
	
	// apply height offset
    o.UV = GetHeightmapUV(GrassPos, PlaneDimension);
    float NoiseVal = Heightmap.SampleLevel(LinearSampler, o.UV, 0.f).r;
    float Height = NoiseVal * HeightDisplacement;
	
    RotatedPos.y *= lerp(0.2f, 1.4f, NoiseVal);
    o.HeightAlongBlade = RotatedPos.y;
	
    o.WorldPos = RotatedPos * 2.f + float3(GrassPos.x, Height, GrassPos.y);
    
    return o;
}

float3 CalculateNormals(uint VertexID, float3 WorldPos, VS_In v)
{
    uint MaxVertID = Grass[v.InstanceID].LOD == 0u ? 10u : 2u;
    
    // assign neighbour vertices
    uint v1, v2;
    if (VertexID == MaxVertID) // tip
    {
        v1 = MaxVertID - 1u;
        v2 = MaxVertID - 2u;
    }
    else if (VertexID % 2u == 0u) // even
    {
        v1 = VertexID + 2u;
        v2 = VertexID + 1u;
    }
    else // odd
    {
        v1 = VertexID - 1u;
        v2 = clamp(VertexID + 2u, VertexID + 1u, MaxVertID);
    }
    
    // get animated world pos
    VS_Out o1 = CalcWorldPos(v, v1);
    VS_Out o2 = CalcWorldPos(v, v2);
    
    float2 GrassPos = Grass[v.InstanceID].Offset;
    Animate(o1, GrassPos, GlobalBuffer.CurrTime, o1.WorldPos);
    Animate(o2, GrassPos, GlobalBuffer.CurrTime, o2.WorldPos);
    
    // calculate tangent vector to neighbour vertices
    float3 t1 = normalize(WorldPos - o1.WorldPos);
    float3 t2 = normalize(WorldPos - o2.WorldPos);
    
    return normalize(cross(t1, t2));
}

VS_Out main(VS_In v)
{
    VS_Out CurrVert = CalcWorldPos(v, v.VertexID);
    float3 PrevWorldPos = CurrVert.WorldPos;
    float2 GrassPos = Grass[v.InstanceID].Offset;
	
    Animate(CurrVert, GrassPos, GlobalBuffer.CurrTime, CurrVert.WorldPos);
    Animate(CurrVert, GrassPos, GlobalBuffer.PrevTime, PrevWorldPos);
	
    CurrVert.WorldNormal = CalculateNormals(v.VertexID, CurrVert.WorldPos, v);
    CurrVert.ViewNormal = normalize(mul(float4(CurrVert.WorldNormal, 0.f), GlobalBuffer.Camera.CurrView).xyz);
	
	CurrVert.Pos = mul(float4(CurrVert.WorldPos, 1.f), GlobalBuffer.Camera.CurrViewProjJittered);
    CurrVert.CurrClipPos = mul(float4(CurrVert.WorldPos, 1.f), GlobalBuffer.Camera.CurrViewProj);
    CurrVert.PrevClipPos = mul(float4(PrevWorldPos, 1.f), GlobalBuffer.Camera.PrevViewProj);
	
	return CurrVert;
}
