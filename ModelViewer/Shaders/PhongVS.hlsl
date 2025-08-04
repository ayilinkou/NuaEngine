cbuffer MatrixBuffer : register(b1)
{
	matrix WorldMatrix;
	matrix ViewMatrix;
	matrix ProjectionMatrix;
};

cbuffer AccumulatedModelTransform : register(b2)
{
	matrix AccumulatedModelMatrix;
}

struct VS_In
{
	float3 Pos : POSITION;
	float2 TexCoord : TEXCOORD0;
	float3 Normal : NORMAL;
};

struct VS_Out
{
	float4 Pos : SV_POSITION;
	float3 WorldPos : POSITION;
	float2 TexCoord : TEXCOORD0;
	float3 WorldNormal : NORMAL;
};

VS_Out main(VS_In v)
{
	VS_Out o;
	
	// mesh vertices have no knowledge whether they are parented to a parent mesh node or not
	// to solve this, multiply by the AccumulatedModelMatrix BEFORE applying model transform
	o.Pos = mul(mul(float4(v.Pos, 1.f), AccumulatedModelMatrix), WorldMatrix);
	
	o.WorldPos = o.Pos.xyz;
	
	o.Pos = mul(o.Pos, ViewMatrix);
	o.Pos = mul(o.Pos, ProjectionMatrix);
	
	o.TexCoord = v.TexCoord;
	
	o.WorldNormal = mul(mul(float4(v.Normal, 0.f), AccumulatedModelMatrix), WorldMatrix).xyz; // correct as long as I use uniform scaling
	o.WorldNormal = normalize(o.WorldNormal);
	
	return o;
}