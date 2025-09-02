#ifndef GLOBALCBUFFER_HLSL
#define GLOBALCBUFFER_HLSL

#define MAX_POINT_LIGHTS 8
#define MAX_DIRECTIONAL_LIGHTS 1
#define MAX_SPOT_LIGHTS 8

struct CameraData
{
    float4x4 CurrView;
    float4x4 CurrProj;
    float4x4 CurrViewProj;
    float4x4 CurrProjJittered;
    float4x4 CurrViewProjJittered;
    float4x4 PrevView;
    float4x4 PrevProj;
    float4x4 PrevViewProj;
    float4x4 PrevProjJittered;
    float4x4 PrevViewProjJittered;
    float4x4 InverseView;
    float4x4 InverseProj;
    float3 ActiveCameraPos;
    float Padding;
    float3 MainCameraPos;
    float Padding1;
    float4 FrustumPlanes[6]; // xyz = normals, w = d
};

struct PointLight
{
    float Radius;
    float3 Pos;
    float SpecularPower;
    float3 Color;
    float Intensity;
    float3 Padding;
};

struct DirectionalLight
{
    float3 Dir;
    float SpecularPower;
    float3 Color;
    float Intensity;
};

struct SpotLight
{
    float Radius;
    float3 Pos;
    float SpecularPower;
    float3 Color;
    float Intensity;
    float3 Dir;
    float CosInnerAngle;
    float CosOuterAngle;
    float2 Padding;
};

struct LightData
{
    PointLight PointLights[MAX_POINT_LIGHTS];
    DirectionalLight DirLights[MAX_DIRECTIONAL_LIGHTS];
    SpotLight SpotLights[MAX_SPOT_LIGHTS];
    float3 SkylightColor;
    int PointLightCount;
    int DirectionalLightCount;
    int SpotLightCount;
    float2 Padding;
};

struct GlobalCBuffer
{
    CameraData Camera;
    LightData Lights;
    float CurrTime;
    float PrevTime;
    float NearZ;
    float FarZ;
    float2 ScreenRes;
    float2 Padding;
};

cbuffer GlobalCBufferInfo : register(b0)
{
    GlobalCBuffer GlobalBuffer;
}

#endif
