#include "Common.hlsl"

StructuredBuffer<CullTransformData> Transforms : register(t0);
StructuredBuffer<float2> Offsets : register(t1);
Texture2D Heightmap : register(t2);

AppendStructuredBuffer<CullTransformData> CulledTransforms : register(u0);
AppendStructuredBuffer<GrassData> CulledGrassData : register(u2);
AppendStructuredBuffer<GrassData> CulledGrassLODData : register(u3);
RWStructuredBuffer<uint> InstanceCounts : register(u4);
RWByteAddressBuffer FirstArgsBuffer : register(u5);
RWByteAddressBuffer SecondArgsBuffer : register(u6);

cbuffer CullData : register(b1)
{
	uint SentInstanceCount;
	uint3 ThreadGroupCounts;
	uint GrassPerChunk;
	uint PlaneDimension;
	float HeightDisplacement;
	float LODDistanceThreshold;
    float3 BBoxMin;
    float Padding;
    float3 BBoxMax;
    float Padding1;
}

bool IsAABBInside(float3 BBoxMin, float3 BBoxMax)
{
    for (int i = 0; i < 6; i++)
    {
        float3 Normal = GlobalBuffer.Camera.FrustumPlanes[i].xyz;
        float3 PosVertex = float3(
            (Normal.x >= 0) ? BBoxMax.x : BBoxMin.x,
            (Normal.y >= 0) ? BBoxMax.y : BBoxMin.y,
            (Normal.z >= 0) ? BBoxMax.z : BBoxMin.z
        );
        
        float Dist = dot(Normal, PosVertex) + GlobalBuffer.Camera.FrustumPlanes[i].w;
        if (Dist < 0.0f)
            return false;
    }
    return true;
}

static const uint tx = 32u;
static const uint ty = 1u;
static const uint tz = 1u;
[numthreads(tx, ty, tz)]
void FrustumCull( uint3 DTid : SV_DispatchThreadID )
{
	uint FlattenedID = DTid.z * ThreadGroupCounts.x * ThreadGroupCounts.y * tx * ty +
                       DTid.y * ThreadGroupCounts.x * tx +
                       DTid.x;
    
    if (FlattenedID >= SentInstanceCount)
        return;
    
    CulledTransforms.Append(Transforms[FlattenedID]);
    InterlockedAdd(InstanceCounts[0], 1u);
    return; // remove when done updating grass culling
	
	const float4x4 t = Transforms[FlattenedID].CurrTransform;
    float3 TransformedMin = mul(float4(BBoxMin, 1.f), t).xyz;
    float3 TransformedMax = mul(float4(BBoxMax, 1.f), t).xyz;

    if (IsAABBInside(TransformedMin, TransformedMax))
    {
        CulledTransforms.Append(Transforms[FlattenedID]);
        InterlockedAdd(InstanceCounts[0], 1u);
        return;
    }
}

static const uint grass_tx = 32u;
static const uint grass_ty = 8u;
static const uint grass_tz = 1u;

[numthreads(grass_tx, grass_ty, grass_tz)]
void FrustumCullGrass(uint3 DTid : SV_DispatchThreadID)
{
	uint GrassID = DTid.x;
	uint ChunkID = DTid.y;
	
	if (GrassID >= GrassPerChunk || ChunkID >= SentInstanceCount)
		return;
	
    const float3 ChunkOffset = mul(float4(0.f, 0.f, 0.f, 1.f), Transforms[ChunkID].CurrTransform).xyz; // culled chunk transforms
	const float3 GrassOffset = float3(Offsets[GrassID].x, 0.f, Offsets[GrassID].y);
	const float3 WorldOffset = float3(ChunkOffset + GrassOffset);
	
	const float2 UV = GetHeightmapUV(WorldOffset.xz, PlaneDimension);
    const float3 Height = float3(0.f, Heightmap.SampleLevel(LinearSampler, UV, 0.f).r * HeightDisplacement, 0.f);
	
	float Dist = distance(GlobalBuffer.Camera.MainCameraPos, WorldOffset.xyz);
	bool bHighLOD = Dist < LODDistanceThreshold;
	
    float3 TransformedMin = BBoxMin + WorldOffset + Height;
    float3 TransformedMax = BBoxMax + WorldOffset + Height;

    // still doesn't work perfectly at high height displacement
    if (IsAABBInside(TransformedMin, TransformedMax))
    {
        GrassData Grass;
        Grass.Offset = WorldOffset.xz;
        Grass.ChunkID = HashFloat2ToUint(ChunkOffset.xz);
			
        if (bHighLOD)
        {
            Grass.LOD = 0u;
            CulledGrassData.Append(Grass);
            InterlockedAdd(InstanceCounts[0], 1u);
            return;
        }
        else
        {
            Grass.LOD = 1u;
            CulledGrassLODData.Append(Grass);
            InterlockedAdd(InstanceCounts[1], 1u);
            return;
        }
    }
}

[numthreads(1, 1, 1)]
void ClearInstanceCount(uint3 DTid : SV_DispatchThreadID)
{
	InstanceCounts[0] = 0u;
	InstanceCounts[1] = 0u;
}

[numthreads(1, 1, 1)]
void TransferInstanceCount(uint3 DTid : SV_DispatchThreadID)
{
	FirstArgsBuffer.Store(4u, InstanceCounts[0]);
	SecondArgsBuffer.Store(4u, InstanceCounts[1]);
}
