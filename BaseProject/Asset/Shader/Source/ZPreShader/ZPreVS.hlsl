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
//[RootSignature(ZPRE_ROOT_SIG)]
[RootSignature(DEFAULT_ROOT_SIG)]

VSOutput vs(VSInput a_input)
{
	// 座標と法線に適用
	VSOutput _out;
	float4 _wPos = mul(mat, float4(a_input.pos,1));
	_out.svpos = mul(cView,_wPos);
	_out.svpos = mul(cProj,_out.svpos);
	
	return _out;
}
