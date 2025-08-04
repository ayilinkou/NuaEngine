Texture2D Heightmap : register(t0);
SamplerState Sampler : register(s0);

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
	float2 UV : TEXCOORD0;
	uint ChunkID : TEXCOORD1;
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

float4 main(PS_In p) : SV_TARGET
{
	if (bVisualiseChunks)
		return float4(RandomRGB(p.ChunkID), 1.f);

	float Height = Heightmap.Sample(Sampler, p.UV).r;
	if (Height < 0.03)
	{
		return float4(50.f / 256.f, 123.f / 256.f, 191.f / 256.f, 1.f);
	}

	return lerp(BotColor, TopColor, Height);
}