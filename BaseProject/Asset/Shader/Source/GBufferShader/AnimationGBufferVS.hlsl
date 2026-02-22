#include "GBufferShader.hlsli"



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

VSOutput vs( VSInput a_input)
{
	// スキニング
	row_major float4x4 _mBones = 0;
	[unroll]
	for (int _i = 0; _i < 4; ++_i)
	{
		_mBones += g_mBones[a_input.skinIndex[_i]] * a_input.skinWeight[_i];
	}

	// 座標と法線に適用
	float4 skinnedPos = mul(float4(a_input.pos, 1), _mBones);
	a_input.pos = skinnedPos.xyz;
	a_input.normal = mul(a_input.normal, (float3x3) _mBones);
	
	// 出力用構造体
	VSOutput _output = (VSOutput) 0; // アウトプット構造体を定義
	// 座標変換
	float4 _pos = float4(a_input.pos, 1);
	_output.pos = mul(_pos, mat); // ワールド行列を掛ける
	_output.wPos = _output.pos.xyz; // ワールド座標を保存
	_output.pos = mul(_output.pos, mView); // ビュー行列を掛ける
	_output.pos = mul(_output.pos, mProj); // 投影行列を掛ける

	// 頂点色
	_output.color = a_input.color; // 頂点色をそのままピクセルシェーダーに渡す

	// 法線
	_output.wN = mul(a_input.normal, (float3x3) mat);

	// 接線
	_output.wT = mul(a_input.tangent, (float3x3) mat);

	// 従法線
	float3 _binormal = cross(a_input.normal, a_input.tangent);
	_output.wB = normalize(mul(_binormal, (float3x3) mat));

	// uv座標
	_output.uv = a_input.uv * uvTransform.zw + uvTransform.xy;
    
	return _output;
}
