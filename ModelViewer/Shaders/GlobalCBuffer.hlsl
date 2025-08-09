#ifndef GLOBALCBUFFER_HLSL
#define GLOBALCBUFFER_HLSL

#define MAX_POINT_LIGHTS 8
#define MAX_DIRECTIONAL_LIGHTS 1

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
    float3 CameraPos;
    float Padding;
};

struct PointLight
{
    float Radius;
    float3 LightPos;
    float SpecularPower;
    float3 LightColor;
};

struct DirectionalLight
{
    float3 LightDir;
    float SpecularPower;
    float3 LightColor;
    float Padding;
};

struct LightData
{
    PointLight PointLights[MAX_POINT_LIGHTS];
    DirectionalLight DirLights[MAX_DIRECTIONAL_LIGHTS];
    float3 SkylightColor;
    int PointLightCount;
    int DirectionalLightCount;
    float3 Padding;
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
