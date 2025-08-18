#include "Common.hlsl"
#include "GlobalCBuffer.hlsl"

Texture2D ScreenTexture : register(t0);
Texture2D DepthTexture : register(t1);
Texture2D NormalTexture : register(t2);
Texture2D NoiseTexture : register(t3);

cbuffer SSAOBuffer : register(b1)
{
    float Radius;
    float3 Padding;
    float3 SampleKernel[64];
};

struct PS_In
{
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PS_In p) : SV_TARGET
{
    const float2 NoiseScale = GlobalBuffer.ScreenRes / 4.f;
    
    float Depth = DepthTexture.Sample(PointClampSampler, p.TexCoord).r;
    float3 Normal = NormalTexture.Sample(PointClampSampler, p.TexCoord).xyz;
    
    float4 hNDC = float4(p.TexCoord.x * 2.f - 1.f, (1.f - (p.TexCoord.y)) * 2.f - 1.f, Depth, 1.f);
    float4 ViewPos = mul(hNDC, GlobalBuffer.Camera.InverseProj);
    ViewPos /= ViewPos.w;
    
    // create TBN change of basis matrix (from tangent space to view space)
    float3 RandomVec = normalize(float3(NoiseTexture.Sample(PointWrapSampler, p.TexCoord * NoiseScale).xy, 0.f));
    float3 Tangent = normalize(RandomVec - Normal * dot(RandomVec, Normal));
    float3 Bitangent = cross(Normal, Tangent);
    float3x3 TBN = float3x3(Tangent, Bitangent, Normal);
    
    float Occlusion = 0.f;
    const float Bias = 0.025f;
    const int KERNEL_SIZE = 64;
    for (int i = 0; i < KERNEL_SIZE; ++i)
    {
        // from tangent to view space
        float3 SampleOffset = mul(SampleKernel[i], TBN);
        
        // view to UV space
        float4 SampleView = float4(ViewPos.xyz + SampleOffset * Radius, 1.f);
        float4 SampleClip = mul(SampleView, GlobalBuffer.Camera.CurrProjJittered);
        SampleClip /= SampleClip.w;
        float2 SampleUV = float2(SampleClip.x * 0.5f + 0.5f, 1.f - (SampleClip.y * 0.5f + 0.5f));
        
        float OccluderDepth = DepthTexture.Sample(PointClampSampler, SampleUV).r;
        float4 OccluderClip = float4(SampleClip.x, SampleClip.y, OccluderDepth, 1.f);
        float4 OccluderView = mul(OccluderClip, GlobalBuffer.Camera.InverseProj);
        OccluderView /= OccluderView.w;
        
        // if occluder is far from the pixel being evaluated, it should not contribute much to the occlusion
        float RangeCheck = smoothstep(0.f, 1.f, Radius / length(ViewPos.xyz - OccluderView.xyz));
        
        Occlusion += (OccluderView.z >= SampleView.z + Bias ? 0.f : 1.f) * RangeCheck;
    }
    
    Occlusion = 1.f - (Occlusion / (float) KERNEL_SIZE);
    return float4(Occlusion, Occlusion, Occlusion, 1.f);
}
