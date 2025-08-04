struct GlobalCBuffer
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
    float CurrTime;
    float PrevTime;
    float NearZ;
    float FarZ;
    float Padding;
};

cbuffer GlobalCBufferInfo : register(b0)
{
    GlobalCBuffer GlobalBuffer;
}