cbuffer CullingBuffer : register(b1)
{
	float4 FrustumPlanes[6];  // left, right, top, bot, near, far // not using right now, but will use when changing to culling my frustum planes rather than clip space coordinates
	float4x4 CullCameraViewProj;
};

struct GS_In
{
	float4 Pos : SV_POSITION;
	float3 WorldPos : WORLDPOS;
	float2 UV : TEXCOORD0;
	uint ChunkID : TEXCOORD1;
    float4 CurrClipPos : TEXCOORD2;
    float4 PrevClipPos : TEXCOORD3;
};

[maxvertexcount(3)]
void main(
	triangle GS_In input[3], 
	inout TriangleStream<GS_In> output
)
{
	bool bInsideFrustum = false;
	const float Bias = 0.5f;

	for (int i = 0; i < 3; ++i)
	{
		float4 Pos = mul(float4(input[i].WorldPos, 1.f), CullCameraViewProj);
		
		// check if at least one vertex is inside the clip space
		if (abs(Pos.x) <= Pos.w + Bias &&
            abs(Pos.y) <= Pos.w + Bias &&
            Pos.z >= -Bias && Pos.z <= Pos.w + Bias)
		{
			bInsideFrustum = true;
			break;
		}
	}

	if (bInsideFrustum)
	{
		for (int i = 0; i < 3; ++i)
		{
			output.Append(input[i]);
		}
	}
}