#include "PBRShader.hlsli"

float4 pixel(VSOutput a_input) : SV_Target
{
	// 出力カラー
	float4 _outColor = { 0.0f, 0.0f, 0.0f, 0.0f };


	// テクスチャカラーにベースカラーと頂点色を乗算
	_outColor = _MainTex.Sample(smp, a_input.uv) * baseColor * a_input.color;
	float3 _specColor = baseColor;

	// カメラへの方向
	float3 _toCam = cCameraPos.xyz - a_input.wPos.xyz;
	float _camDist = length(_toCam);	// カメラまでの距離
	_toCam = normalize(_toCam);			// 正規化

	// 法線マップから法線ベクトル取得
	row_major float3x3 mTBN =
	{
		normalize(a_input.wT),
		normalize(a_input.wB),
		normalize(a_input.wN)
	};

	

	


	
	return _outColor;
}
