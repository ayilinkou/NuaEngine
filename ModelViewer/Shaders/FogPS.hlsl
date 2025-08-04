Texture2D screenTexture : register(t0);
Texture2D depthTexture : register(t1);
SamplerState samplerState : register(s0);

#define LINEAR 0
#define EXPONENTIAL 1
#define EXPONENTIAL_SQUARED 2

cbuffer FogBuffer : register(b1)
{
	float3 FogColor;
	int Formula;
	float Density;
	float NearPlane;
	float FarPlane;
	float Padding;
};

struct PS_In
{
	float4 Pos : SV_POSITION;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD0;
};

float LinearFog(float Distance, float Near, float Far)
{
	return (Far - Distance) / (Far - Near);
}

float ExponentialFog(float Distance)
{
	return pow(2, -(Distance * Density));
}

float ExponentialSquaredFog(float Distance)
{
	return pow(2, -pow(Distance * Density, 2));
}

float4 main(PS_In p) : SV_TARGET
{
	float3 Color = screenTexture.Sample(samplerState, p.TexCoord).xyz;
	
	float NonLinearDepth = depthTexture.Sample(samplerState, p.TexCoord);

	float LinearDepth = (1.f - FarPlane / NearPlane) * NonLinearDepth + (FarPlane / NearPlane);
	LinearDepth = 1.f / LinearDepth;
	float ViewDistance = LinearDepth * FarPlane;
	
	float FogFactor = 0.f;
	if (Formula == LINEAR)
	{
		FogFactor = LinearFog(ViewDistance, NearPlane, FarPlane);
	}
	else if (Formula == EXPONENTIAL)
	{
		FogFactor = ExponentialFog(ViewDistance);
	}
	else if (Formula == EXPONENTIAL_SQUARED)
	{
		FogFactor = ExponentialSquaredFog(ViewDistance);
	}
	
	return float4(lerp(FogColor, Color, FogFactor), 1.f);
}