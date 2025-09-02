#include "GlobalCBuffer.hlsl"
#include "Common.hlsl"

Texture2D DiffuseTexture : register(t0);
Texture2D NormalSpecTexture : register(t1);
Texture2D SSAOTexture : register(t2);
Texture2D DepthTexture : register(t3);

struct PS_In
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PS_In p) : SV_TARGET
{
    float3 Diffuse = DiffuseTexture.Sample(PointClampSampler, p.TexCoord).rgb;
    float4 NormalSpec = NormalSpecTexture.Sample(PointClampSampler, p.TexCoord);
    float3 ViewNormal = NormalSpec.xyz;
    float Reflectance = NormalSpec.a;
    float Depth = DepthTexture.Sample(PointClampSampler, p.TexCoord).r;
    float Visibility = SSAOTexture.Sample(PointClampSampler, p.TexCoord).r;
    
    float4 hNDC = float4(p.TexCoord.x * 2.f - 1.f, (1.f - (p.TexCoord.y)) * 2.f - 1.f, Depth, 1.f);
    float4 ViewPos = mul(hNDC, GlobalBuffer.Camera.InverseProj);
    ViewPos /= ViewPos.w;
    float3 WorldPos = mul(ViewPos, GlobalBuffer.Camera.InverseView).xyz;
    float3 WorldNormal = normalize(mul(float4(ViewNormal, 0.f), GlobalBuffer.Camera.InverseView).xyz);
    float3 PixelToCam = normalize(GlobalBuffer.Camera.ActiveCameraPos - WorldPos);
    
    float3 Light = CalcLights(Diffuse, WorldPos, WorldNormal, PixelToCam, Reflectance).rgb;
	
    float3 Ambient = float3(0.3f, 0.3f, 0.3f) * Diffuse * Visibility;
	return float4(Light + Ambient, 1.f);
}