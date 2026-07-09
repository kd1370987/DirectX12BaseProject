#include "Skinning.hlsli"

[RootSignature(SKINNING_ROOT_SIG)]

[numthreads(64, 1, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
	// 範囲外アクセスチェック : 頂点数を超えたスレッドは終了
	//if (DTid.x >= g_info.vertexCount) return;
	
	// メガバッファ上の絶対インデックス
	uint _globalVertIdx = g_info.vertexStart + DTid.x;

	// 頂点データの取得
	Vertex _vert = g_vertexfloatData[_globalVertIdx];

	uint _skinIndex[4];
	_skinIndex[0] = _vert.skinIndex.x & 0xFFFF;
	_skinIndex[1] = (_vert.skinIndex.x >> 16) & 0xFFFF;
	_skinIndex[2] = _vert.skinIndex.y & 0xFFFF;
	_skinIndex[3] = (_vert.skinIndex.y >> 16) & 0xFFFF;

	// ---- スキニング計算 ----
	row_major float4x4 _mBones = 0;
	[unroll]
	for (int _i = 0; _i < 4; ++_i)
	{
		_mBones += g_bonePalletData[g_info.boneOffset + _skinIndex[_i]].mat * _vert.skinWeight[_i];
	}
	// 座標と法線
	float4 _skinnedPos = mul(float4(_vert.pos, 1), _mBones);
	float3 _skinnedNormal = mul(_vert.normal, (float3x3) _mBones);
	float3 _skinnedTangent = mul(_vert.tangent, (float3x3) _mBones);
	
	// 新たな頂点を入れる
	Vertex _outVert;
	_outVert = _vert;
	_outVert.pos = _skinnedPos.xyz;
	_outVert.normal = _skinnedNormal;
	_outVert.tangent = normalize(_skinnedTangent);
	
	g_outputVertex[g_info.animatedVertStart + DTid.x] = _outVert;
}
