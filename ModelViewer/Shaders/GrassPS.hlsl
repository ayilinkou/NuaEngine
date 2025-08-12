#include "Common.hlsl"
#include "GlobalCBuffer.hlsl"

struct PS_In
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

struct PS_Out
{
    float4 Color : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float2 Velocity : SV_TARGET2;
};

static const float3 AOColor		= float3(0.0f,  0.2f, 0.0f);
static const float3 RootColor	= float3(0.0f,  0.3f, 0.0f);
static const float3 MidColor	= float3(0.4f,  0.7f, 0.1f);
static const float3 TipColor	= float3(0.9f,  1.0f, 0.2f);

PS_Out main(PS_In p)
{
    PS_Out o;
	
	/*if (p.LOD == 1u)
	{
		o.Color = float4(1.f, 0.f, 0.f, 1.f);
        o.Normal = float4(p.ViewNormal, 0.f);
        o.Velocity = CalculateMotionVector(p.CurrClipPos, p.PrevClipPos);
        return 0;
    }*/
	
    float3 Color = MidColor;
    float3 PixelToCam = normalize(GlobalBuffer.Camera.CameraPos - p.WorldPos);
    float4 LightTotal = float4(0.f, 0.f, 0.f, 0.f);
	
	float TipThreshold = 2.f;
	float MidThreshold = 1.f;
	float RootThreshold = 0.2f;
	float AOThreshold = 0.f;
	
	if (p.HeightAlongBlade > MidThreshold)
	{
		Color = lerp(MidColor, TipColor, Remap(p.HeightAlongBlade, MidThreshold, TipThreshold, 0.f, 1.f));
	}
	else if (p.HeightAlongBlade > RootThreshold)
	{
		Color = lerp(RootColor, MidColor, Remap(p.HeightAlongBlade, RootThreshold, MidThreshold, 0.f, 1.f));
	}
	else if (p.HeightAlongBlade > AOThreshold)
	{
		Color = lerp(AOColor, RootColor, Remap(p.HeightAlongBlade, AOThreshold, RootThreshold, 0.f, 1.f));
	}
				
    for (int i = 0; i < GlobalBuffer.Lights.DirectionalLightCount; i++)
    {
        // to simulate subsurface scaterring, we will light the grass on both sides, flipping the normal vector if needed
		float3 AdjustedNormal = p.WorldNormal;
        if (dot(-GlobalBuffer.Lights.DirLights[i].LightDir, AdjustedNormal) < 0.f)
		{
			AdjustedNormal = -p.WorldNormal;
		}
			
		float DiffuseFactor = saturate(dot(-GlobalBuffer.Lights.DirLights[i].LightDir, AdjustedNormal));
        DiffuseFactor = Remap(DiffuseFactor, 0.f, 1.f, 0.5f, 1.f); // indirectly adding ambient to pixels perpendicular to light dir
        float4 Diffuse = float4(GlobalBuffer.Lights.DirLights[i].LightColor, 1.f) * float4(Color, 0.5f) * DiffuseFactor;
        LightTotal += Diffuse;

		// using the original world normal to determine which side to add specular
        float3 HalfwayVec = normalize(PixelToCam - GlobalBuffer.Lights.DirLights[i].LightDir);
        float SpecularFactor = pow(saturate(dot(p.WorldNormal, HalfwayVec)), GlobalBuffer.Lights.DirLights[i].SpecularPower);
		float4 Specular = float4(GlobalBuffer.Lights.DirLights[i].LightColor, 1.f) * SpecularFactor;
		LightTotal += Specular;
    }
	
    for (int i = 0; i < GlobalBuffer.Lights.PointLightCount; i++)
    {
        float Distance = distance(p.WorldPos, GlobalBuffer.Lights.PointLights[i].LightPos);
        float Attenuation = saturate(1.f - (Distance * Distance) / (GlobalBuffer.Lights.PointLights[i].Radius *GlobalBuffer.Lights.PointLights[i].Radius)); // less control than constant, linear and quadratic, but guaranteed to reach 0 past max radius
	
		// to simulate subsurface scaterring, we will light the grass on both sides, flipping the normal vector if needed
        float3 AdjustedNormal = p.WorldNormal;
        float3 PixelToLight = normalize(GlobalBuffer.Lights.PointLights[i].LightPos - p.WorldPos);
        if (dot(PixelToLight, AdjustedNormal) < 0.f)
        {
            AdjustedNormal = -p.WorldNormal;
        }
		
        float DiffuseFactor = saturate(dot(PixelToLight, AdjustedNormal));
        float4 Diffuse = float4(GlobalBuffer.Lights.PointLights[i].LightColor, 1.f) * float4(Color.xyz, 0.5f) * DiffuseFactor;
        LightTotal += Diffuse * Attenuation;
		
        float3 HalfwayVec = normalize(PixelToCam + PixelToLight);
        float SpecularFactor = pow(saturate(dot(p.WorldNormal, HalfwayVec)), GlobalBuffer.Lights.PointLights[i].SpecularPower);
        float4 Specular = float4(GlobalBuffer.Lights.PointLights[i].LightColor, 1.f) * SpecularFactor;
        LightTotal += Specular * Attenuation;
    }
		
	float AmbientFactor = 0.5f;
	float4 Ambient = float4((Color * GlobalBuffer.Lights.SkylightColor), 1.f) * AmbientFactor;
		
    o.Color = Ambient + LightTotal;
    //o.Color = float4(p.WorldNormal, 1.f);
    o.Normal = float4(p.ViewNormal, 0.f);
    o.Velocity = CalculateMotionVector(p.CurrClipPos, p.PrevClipPos);
	
    return o;
}
