Texture2D screenTexture : register(t0);
SamplerState samplerState : register(s0);

cbuffer ToneMapperBuffer : register(b1)
{
	float whiteLevel;
	float exposure;
	float bias;
	int formula;
};

#define REINHARD_BASIC 0
#define REINHARD_EXTENDED 1
#define REINHARD_EXTENDED_BIAS 2
#define NARKOWICZ_ACES 3
#define HILL_ACES 4

struct PS_In
{
	float4 Pos : SV_POSITION;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD0;
};

float3 ReinhardBasic(float3 color)
{
	return color / (1.f + color);
}

float3 ReinhardExtended(float3 color)
{
	float3 numerator = color * (1.f + color / (whiteLevel * whiteLevel));
	float3 denominator = 1.f + color;
	return numerator / denominator;
}

float3 ReinhardExtendedBias(float3 color)
{
	return color / (color + bias);
}

float3 NarkowiczACES(float3 color)
{
	const float a = 2.51f;
	const float b = 0.03f;
	const float c = 2.43f;
	const float d = 0.59f;
	const float e = 0.14f;
	return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

static const float3x3 ACESInputMat =
{
	{ 0.59719, 0.35458, 0.04823 },
	{ 0.07600, 0.90834, 0.01566 },
	{ 0.02840, 0.13383, 0.83777 }
};

static const float3x3 ACESOutputMat =
{
	{  1.60475, -0.53108, -0.07367 },
	{ -0.10208,  1.10813, -0.00605 },
	{ -0.00327, -0.07276,  1.07602 }
};

float3 RRTAndODTFit(float3 color)
{
	float3 a = color * (color + 0.0245786f) - 0.000090537f;
	float3 b = color * (0.983729f * color + 0.4329510f) + 0.238081f;
	return a / b;
}

float3 HillACES(float3 color)
{
	float3 result = mul(ACESInputMat, color);
	result = RRTAndODTFit(result);
	result = mul(ACESOutputMat, result);

	return saturate(result);
}

float4 main(PS_In p) : SV_TARGET
{
	float4 color = screenTexture.Sample(samplerState, p.TexCoord);
	float3 toneMappedColor = float3(0.f, 0.f, 0.f);
	
	if (formula == REINHARD_BASIC)
	{
		toneMappedColor = ReinhardBasic(color.xyz);
	}
	else if (formula == REINHARD_EXTENDED)
	{
		toneMappedColor = ReinhardExtended(color.xyz);
	}
	else if (formula == REINHARD_EXTENDED_BIAS)
	{
		toneMappedColor = ReinhardExtendedBias(color.xyz);
	}
	else if (formula == NARKOWICZ_ACES)
	{
		toneMappedColor = NarkowiczACES(color.xyz);
	}
	else if (formula == HILL_ACES)
	{
		toneMappedColor = HillACES(color.xyz);
	}
	
	return float4(toneMappedColor, 1.f);
}
