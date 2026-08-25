//==========================================================================================
//
// BloomCompositeShader
//
// メインカラーへ、まとめ終わったブルームを加算合成する。
//
//   出力 = メインカラー + ブルーム * intensity
//
// トーンマップ前のHDR段階で足すので、加算した結果が1.0を超えても構わない。
// 最終的に FullScreenPass のトーンマップが拾って、白飛び側へなめらかに収めてくれる。
// （トーンマップ後に足すと、加算したぶんがそのままクランプされて板のような白になる）
//
// 無効時はメインカラーをそのまま素通しするので、絵は変わらない。
//
//==========================================================================================
#include "../../../Common/RootSignatureLayout.hlsli"

#include "../../../Common/RootParameters/BloomOptionData.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b13)           ブルーム設定
//   1 : SRVテーブル(t0-t1) メインカラー + ブルーム
//   2 : UAVテーブル(u0)    合成結果
//==========================================================================================
#define BLOOM_COMPOSITE_RS \
"RootFlags(0)," \
"CBV(b13, visibility = SHADER_VISIBILITY_ALL)," \
"DescriptorTable(SRV(t0, numDescriptors=2)), " \
"DescriptorTable(UAV(u0, numDescriptors=1))"

cbuffer CBBloomOption : register(b13)
{
	BloomOptionData g_bloom;
}

// 入力
Texture2D<float4> g_colorTex : register(t0);	// メインカラー
Texture2D<float4> g_bloomTex : register(t1);	// まとめ終わったブルーム

// 出力
RWTexture2D<float4> g_outTex : register(u0);

[RootSignature(BLOOM_COMPOSITE_RS)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// 画像の縦横サイズを取得
	uint _width, _height;
	g_outTex.GetDimensions(_width, _height);

	// 画面外ならリターン
	if (DTid.x >= _width || DTid.y >= _height) return;

	float4 _mainColor = g_colorTex[DTid.xy];

	// 無効ならそのまま通す
	if (g_bloom.enable == 0)
	{
		g_outTex[DTid.xy] = _mainColor;
		return;
	}

	float3 _bloomColor = g_bloomTex[DTid.xy].rgb;

	// アルファはメインカラーのものを保つ（後段のUIやコピーが見ている）
	g_outTex[DTid.xy] = float4(_mainColor.rgb + _bloomColor * g_bloom.intensity, _mainColor.a);
}
