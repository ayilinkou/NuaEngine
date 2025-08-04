cbuffer ViewProjBuffer : register(b1)
{
	float4x4 ViewProj;
};

struct VSOut
{
	float3 WorldPos : TEXCOORD0;
	float4 Pos : SV_Position;
};

VSOut main(float3 pos : POSITION )
{
	VSOut o;
	
	o.WorldPos = pos;
	o.Pos = mul(float4(pos, 1.f), ViewProj).xyww;
	
	return o;
}