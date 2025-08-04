Texture2D screenTexture : register(t0);
SamplerState samplerState : register(s0);

cbuffer ColorData : register(b1)
{
	float Contrast;
	float Brightness;
	float Saturation;
	float Padding;
};

struct PS_In
{
	float4 Pos : SV_POSITION;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD0;
};

float4 main(PS_In p) : SV_TARGET
{
	float3 Color = screenTexture.Sample(samplerState, p.TexCoord).xyz;
	float Greyscale = 0.299f * Color.r + 0.587f * Color.g + 0.114f * Color.b;
	float3 GreyscaleColor = float3(Greyscale, Greyscale, Greyscale);
	
	Color = Contrast * (Color - 0.5f) + 0.5f;
	Color += Brightness;
	
	return float4(saturate(lerp(GreyscaleColor, Color, Saturation)), 1.f);
}