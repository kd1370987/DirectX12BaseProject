#include "ForwardLightingRoot.hlsli"

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
[RootSignature(FOWARDLIGHTING_ROOT_SIG)]

VSOutput vs( VSInput a_input )
{
	VSOutput _out;

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
    
	float4 _localPos = float4(a_input.pos, 1.0f); // 頂点座標
	float4 _worldPos = mul(mat, _localPos); // ワールド座標に変換
	float4 _viewPos = mul(cView, _worldPos); // ビュー座標に変換
	float4 _projPos = mul(cProj, _viewPos); // 投影変換
    
	_output.svpos = _projPos; // 投影変換された座標をピクセルシェーダーに渡す
	_output.color = a_input.color; // 頂点色をそのままピクセルシェーダーに渡す
	_output.uv = a_input.uv; // uv座標をそのままピクセルシェーダーに渡す
	_output.normal = a_input.normal; // 法線をそのままピクセルシェーダーに渡す

		// ワールド法線、接線、副接線
	float3x3 _normalMat = (float3x3) mat; // ワールド行列の上位3x3部分を取得
	_output.wN = normalize(mul((float3x3) _normalMat, a_input.normal));
	_output.wT = normalize(mul((float3x3) mat, a_input.tangent.xyz));

	float3 _binormal = cross(a_input.normal, a_input.tangent.xyz);
	_output.wB = normalize(mul((float3x3) mat, _binormal));
    
	return _output;
	
	return _out;
}
