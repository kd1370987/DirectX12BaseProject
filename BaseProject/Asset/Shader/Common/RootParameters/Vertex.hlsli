// メガバッファへ積む頂点。ラスタライザ・メッシュシェーダー・レイトレ・
// スキニングがすべてこの並びで読むので、変えるときは頂点を作る側も同時に直すこと
#ifndef ROOTPARAM_VERTEX_HLSLI
#define ROOTPARAM_VERTEX_HLSLI

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
