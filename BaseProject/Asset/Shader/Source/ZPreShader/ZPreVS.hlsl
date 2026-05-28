#include "ZPreShader.hlsli"

// 頂点シェーダー入出力構造体
struct VSInput
{
	float3 pos : POSITION; // 頂点座標
	float3 normal : NORMAL; // 法線
	float2 uv : TEXCOORD; // uv座標
	float3 tangent : TANGENT; // 接空間
	float4 color : COLOR; // 頂点色
};

// ルートシグネチャ定義
[RootSignature(ZPRE_ROOT_SIG)]

VSOutput vs(VSInput a_input)
{
	int _index = g_bufferIndex.instanceDataIndex;
	
	VSOutput _out;
	_out.svpos = Transform_LocalToProj(a_input.pos, g_instanceData[_index].worldMat);
	return _out;
}
