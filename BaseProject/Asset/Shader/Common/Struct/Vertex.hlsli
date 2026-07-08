// インクルードガード
#ifndef VERTEX_HLSLI
#define VERTEX_HLSLI

struct Vertex
{
	float3 pos;
	float3 normal;
	float2 uv;
	float3 tangent;
	float4 color;
		
	uint2 skinIndex;
	float4 skinWeight;
};

#endif

