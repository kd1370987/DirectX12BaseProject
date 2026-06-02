#include "../../Source/CalcNormal.hlsli"

// 入力
Texture2D<float4> g_currentGITex : register(t0);	// 現在のGI
Texture2D<float4> g_velocityTex : register(t1);		// モーションベクター
Texture2D<float4> g_historyGITex : register(t2);	// 前フレームのGI
Texture2D<float4> g_depthTex : register(t3);		// 現在深度
Texture2D<float4> g_normalTex : register(t4);		// 現在法線
Texture2D<float4> g_prevDepthTex : register(t5);	// 過去深度
Texture2D<float4> g_prevNormalTex : register(t6);	// 過去法線

// 出力
RWTexture2D<float4> g_outputGI : register(u0);		// 結果書き込み用

// サンプラー
SamplerState g_smp : register(s0);

// ---- DTid ----
// ディスパッチ全体でこのスレッドは何番目かの変数
// このシェーダーではテクスチャを処理するため今のピクセル座標となる
[numthreads(8, 8, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
	// 画像の解像度を取得
	uint _width, _height;
	g_outputGI.GetDimensions(_width, _height);

	if (DTid.x >= _width || DTid.y >= _height) return;

	// UV座標の計算（ピクセルの中心をサンプリングするため +0.5f）
	float2 _uv = (DTid.xy + 0.5f) / float2(_width, _height);

	// 現在の情報を取得
	float4 _currentGI = g_currentGITex.Load(int3(DTid.xy,0));
	float3 _currentNormal = DecsodeNormal(g_normalTex.Load(int3(DTid.xy, 0)).rg);
	float _currentDepth = g_depthTex.Load(int3(DTid.xy,0)).r;

	// モーションベクターを取得し、過去のUVを逆算
	float2 _velocity = g_velocityTex.Load(int3(DTid.xy,0)).xy;
	float2 _prevUV = _uv - _velocity;

	// 画面外チェック
	if (_prevUV.x < 0.0f || _prevUV.x > 1.0f || _prevUV.y < 0.0f || _prevUV.y > 1.0f)
	{
		// 画面外から入ってきた新しいピクセルは過去の履歴がないのでそのまま出力
		g_outputGI[DTid.xy] = _currentGI;
		return;
	}

	// 過去の履歴をバイリニアサンプリング（ずれたUVから色を保管してもらう）
	float4 _historyGI = g_historyGITex.SampleLevel(g_smp, _prevUV, 0);
	float3 _prevNormal = DecsodeNormal(g_prevNormalTex.SampleLevel(g_smp,_prevUV,0).rg);
	float _prevDepth = g_prevDepthTex.SampleLevel(g_smp, _prevUV, 0).r;

	// -------------------------------------------------------------------------------
	// ゴースト対策（ディスオクルージョン判定）
	// これまで隠れていた背景や物体が新たに露出する現象を検出する
	bool _isValidHistory = true;

	// 法線の向きが違いすぎる場合は履歴を捨てる
	if (dot(_currentNormal,_prevNormal) < 0.8f)
	{
		_isValidHistory = false;
	}

	// 深度が違いすぎる場合は履歴を捨てる
	// いったん深度値で比較。ワールド座標での比較に変更予定
	if (abs(_currentDepth - _prevDepth) > 0.01f)
	{
		_isValidHistory = false;
	}
	
	// -------------------------------------------------------------------------------
	// ブレンド処理
	float4 _finalGI = _currentGI;

	if(_isValidHistory)
	{
		// 履歴が有効ならブレンド（現在 10% : 過去 90%）
		// アルファが小さいほどノイズは消えるが、動いた時の残像が出やすくなる
		float _alpha = 0.1f;
		_finalGI = lerp(_historyGI, _currentGI, _alpha);
	}

	// 結果を出力（RGBに色、AにHitDistance）
	g_outputGI[DTid.xy] = _finalGI;
}
