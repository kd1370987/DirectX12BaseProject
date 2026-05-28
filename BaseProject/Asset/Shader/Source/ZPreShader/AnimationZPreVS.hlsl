#include "ZPreShader.hlsli"

// 頂点シェーダー入出力構造体
struct VSInput
{
	float3 pos : POSITION; // 頂点座標
	float3 normal : NORMAL; // 法線
	float2 uv : TEXCOORD; // uv座標
	float3 tangent : TANGENT; // 接空間
	float4 color : COLOR; // 頂点色
	uint4 skinIndex : SKININDEX; // スキンメッシュのボーンインデックス（何番目のボーンに影響しているかのデータ（最大４））
	float4 skinWeight : SKINWEIGHT; // ボーンの影響度（最大４）
};

// ルートシグネチャ定義
[RootSignature(ZPRE_ROOT_SIG)]

VSOutput vs(VSInput a_input)
{
	int _index = g_bufferIndex.instanceDataIndex;
	float4x4 _worldMat = g_instanceData[_index].worldMat;
	// スキニング
	row_major float4x4 _mBones = 0;
	[unroll]
	for (int _i = 0; _i < 4; ++_i)
	{
		_mBones += g_bonePalletData[g_instanceData[_index].boneStartIndex + a_input.skinIndex[_i]].mat * a_input.skinWeight[_i];
	}

	// 座標と法線に適用
	float4 skinnedPos = mul(float4(a_input.pos, 1), _mBones);
	a_input.pos = skinnedPos.xyz;
	
	VSOutput _out;
	_out.svpos = Transform_LocalToProj(a_input.pos, _worldMat);
	return _out;
}
