// if changing these, also update in Common.h

#define MAX_POINT_LIGHTS 8
#define MAX_DIRECTIONAL_LIGHTS 1
#define MAX_PLANE_CHUNKS 1024
#define MAX_GRASS_PER_CHUNK 10000
#define MAX_INSTANCE_COUNT 1024

struct CameraInfo
{
    float4x4 View;
    float4x4 Proj;
    float4x4 ViewProj;
    float3 Pos;
    float Padding;
};

struct GrassData
{
	float2 Offset;
	uint ChunkID;
	uint LOD;
};

float Remap(float Value, float FromMin, float FromMax, float ToMin, float ToMax)
{
	return ToMin + (Value - FromMin) * (ToMax - ToMin) / (FromMax - FromMin);
}

float2 GetHeightmapUV(float2 Pos, float PlaneDimension)
{
	float HalfPlaneDimension = PlaneDimension / 2.f;
	float x = Remap(Pos.x, -HalfPlaneDimension, HalfPlaneDimension, 0.f, 1.f);
	float y = Remap(Pos.y, -HalfPlaneDimension, HalfPlaneDimension, 0.f, 1.f);
	y = 1.f - y;
	
	return float2(x, y);
}

float2 Rotate(float2 v, float angle)
{
	float s = sin(angle);
	float c = cos(angle);
	return float2(c * v.x - s * v.y, s * v.x + c * v.y);
}

float3 Rotate(float3 v, float3 axis, float angle)
{
	axis = normalize(axis);

	float cosTheta = cos(angle);
	float sinTheta = sin(angle);

	return v * cosTheta +
           cross(axis, v) * sinTheta +
           axis * dot(axis, v) * (1.0 - cosTheta);
}

float2 SumOfSines(float Value, float2 WindDir, float FreqMultiplier, float AmpMultipler, uint Count, float Phase)
{
	float2 Sum = float2(0.f, 0.f);
	float Freq = 1.f;
	float Amp = 1.f;
	float Sign = 1.f;
	
	for (uint i = 0; i < Count; i++)
	{
		float AngleOffset = (float)i * 0.25f * Sign;
		float2 Dir = Rotate(WindDir, AngleOffset);
		
		Sum += sin(Value * Freq + Phase * (float)i) * Amp * Dir;
		Freq *= FreqMultiplier;
		Amp *= AmpMultipler;
		Sign *= -1.f;
	}

	return Sum;
}

uint GenerateChunkID(float2 v)
{
	int2 i = int2(v * 65536.0f);
	uint hash = uint(i.x) * 73856093u ^ uint(i.y) * 19349663u;
	hash ^= (hash >> 13);
	hash *= 0x85ebca6bu;
	hash ^= (hash >> 16);

	return hash;
}

float2 __fade(float2 t)
{
	return t * t * t * (t * (t * 6 - 15) + 10);
}

float Hash(float n)
{
	return frac(sin(n) * 43758.5453);
}

float2 Hash(float2 p)
{
    // Simple hash to generate pseudo-random gradients
	p = float2(dot(p, float2(127.1, 311.7)),
               dot(p, float2(269.5, 183.3)));
	return frac(sin(p) * 43758.5453);
}

uint HashFloat2ToUint(float2 p)
{
    // Large primes for hashing
	const float2 k = float2(127.1, 311.7);

    // Dot product and sine scramble
	float n = dot(p, k);
	return asuint(frac(sin(n) * 43758.5453));
}

float __grad(float2 hash, float2 pos)
{
    // Convert hash to gradient direction (-1 to 1)
	float2 grad = normalize(hash * 2.0 - 1.0);
	return dot(grad, pos);
}

float PerlinNoise(float2 p)
{
	float2 pi = floor(p);
	float2 pf = frac(p);

    // Get hash gradients for the 4 corners
	float2 h00 = Hash(pi + float2(0.0, 0.0));
	float2 h10 = Hash(pi + float2(1.0, 0.0));
	float2 h01 = Hash(pi + float2(0.0, 1.0));
	float2 h11 = Hash(pi + float2(1.0, 1.0));

    // Compute dot products
	float d00 = __grad(h00, pf - float2(0.0, 0.0));
	float d10 = __grad(h10, pf - float2(1.0, 0.0));
	float d01 = __grad(h01, pf - float2(0.0, 1.0));
	float d11 = __grad(h11, pf - float2(1.0, 1.0));

    // Interpolate
	float2 f = __fade(pf);
	float x1 = lerp(d00, d10, f.x);
	float x2 = lerp(d01, d11, f.x);
	return lerp(x1, x2, f.y);
}

float2 PerlinNoise2D(float2 p)
{
    // Grid cell coordinates
	float2 i = floor(p);
	float2 f = frac(p);

    // Fade curve
	float2 u = __fade(f);

    // Hash corners of the cell
	float2 h00 = Hash(i + float2(0.0, 0.0));
	float2 h10 = Hash(i + float2(1.0, 0.0));
	float2 h01 = Hash(i + float2(0.0, 1.0));
	float2 h11 = Hash(i + float2(1.0, 1.0));

    // Compute gradient dot products at corners
	float d00 = __grad(h00, f - float2(0.0, 0.0));
	float d10 = __grad(h10, f - float2(1.0, 0.0));
	float d01 = __grad(h01, f - float2(0.0, 1.0));
	float d11 = __grad(h11, f - float2(1.0, 1.0));

    // Interpolate
	float x0 = lerp(d00, d10, u.x);
	float x1 = lerp(d01, d11, u.x);
	float final = lerp(x0, x1, u.y);

    // Generate a 2D vector output by hashing again and scaling by final noise
	float2 dir = normalize(Hash(i) * 2.0 - 1.0);
	return dir * final;
}

float RandomAngle(float2 seed)
{
    // Returns a random angle in [0, 2 pi)
	return Hash(seed.x + seed.y) * 2.0 * 3.14159265;
}
