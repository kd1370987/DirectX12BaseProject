// インクルードガード
#ifndef LIGHTING_FOG_HLSLI
#define LIGHTING_FOG_HLSLI

//==========================================================================================
//
// Fog
//
// アンビエントCB(AmbientData)のフォグ設定を使って、ライティング結果へフォグを掛ける。
// レイマーチはせず「深度(と高さ)を見て線形に濃くしていくだけ」の軽い実装。
//
//   出現位置 …… 0%(フォグなし)
//   マックスレンジ …… 100%(フォグ色一色)
//   その間 …… 線形グラデーション
//   マックスレンジより先 …… 100% のまま
//
// enable が 0 のときは計算ごとスキップする(条件はディスパッチ全体で同じなので分岐は軽い)。
//
//==========================================================================================
#include "../RootParameters/AmbientData.hlsli"

// フォグの掛かり具合(0..1)を求める
//   a_distance : 出現位置からの距離(手前なら負)
//   a_range    : 100% になるまでの距離(グラデーションの幅)
float CalcFogFactor(float a_distance, float a_range)
{
	// 幅が無い設定なら、出現位置を越えた瞬間に 100%
	if (a_range <= 1e-4f) return (a_distance > 0.0f) ? 1.0f : 0.0f;

	return saturate(a_distance / a_range);
}

//------------------------------------------------------------------------------------------
// 距離フォグ
//   a_viewDepth : カメラからの深度(ビュー空間Z)
//------------------------------------------------------------------------------------------
float3 ApplyDistanceFog(float3 a_color, float a_viewDepth)
{
	if (g_ambient.distanceFogEnable == 0) return a_color;

	float _factor = CalcFogFactor(
		a_viewDepth - g_ambient.distanceFogStart,
		g_ambient.distanceFogMaxRange - g_ambient.distanceFogStart);

	return lerp(a_color, g_ambient.distanceFogColor, _factor);
}

//------------------------------------------------------------------------------------------
// 高さフォグ
//   a_worldPosY : ピクセルのワールド座標Y
//
// heightFogHeight を境に、denseDown で指定した側へ heightFogMaxRange 進むと 100%。
// 反対側には掛からない(距離が負になるので factor が 0)。
//------------------------------------------------------------------------------------------
float3 ApplyHeightFog(float3 a_color, float a_worldPosY)
{
	if (g_ambient.heightFogEnable == 0) return a_color;

	// 濃くなる側への距離
	float _depth = (g_ambient.heightFogDenseDown != 0)
		? (g_ambient.heightFogHeight - a_worldPosY)		// 下へ行くほど濃い
		: (a_worldPosY - g_ambient.heightFogHeight);	// 上へ行くほど濃い

	float _factor = CalcFogFactor(_depth, g_ambient.heightFogMaxRange);

	return lerp(a_color, g_ambient.heightFogColor, _factor);
}

//------------------------------------------------------------------------------------------
// 距離フォグ → 高さフォグ の順で掛ける
//------------------------------------------------------------------------------------------
float3 ApplyFog(float3 a_color, float a_viewDepth, float a_worldPosY)
{
	float3 _color = ApplyDistanceFog(a_color, a_viewDepth);
	_color = ApplyHeightFog(_color, a_worldPosY);
	return _color;
}

#endif
