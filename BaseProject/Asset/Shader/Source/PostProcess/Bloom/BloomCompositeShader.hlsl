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
//   1 : SRVの番号(t0-t1) メインカラー + ブルーム
//   2 : UAVの番号(u0)    合成結果
//==========================================================================================
#define BLOOM_COMPOSITE_RS \
"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
"CBV(b13, visibility = SHADER_VISIBILITY_ALL)," \
"RootConstants(num32BitConstants=2, b100), " \
"RootConstants(num32BitConstants=1, b101)"

cbuffer CBBloomOption : register(b13)
{
	BloomOptionData g_bloom;
}

// 入力
// SRVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex0 : register(b100)
{
	uint g_colorTexIndex;
	uint g_bloomTexIndex;
}

Texture2D<float4> Get_colorTex() { Texture2D<float4> _r = ResourceDescriptorHeap[g_colorTexIndex]; return _r; }	// メインカラー
#define g_colorTex Get_colorTex()
Texture2D<float4> Get_bloomTex() { Texture2D<float4> _r = ResourceDescriptorHeap[g_bloomTexIndex]; return _r; }	// まとめ終わったブルーム
#define g_bloomTex Get_bloomTex()

// 出力
// UAVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex1 : register(b101)
{
	uint g_outTexIndex;
}

RWTexture2D<float4> Get_outTex() { RWTexture2D<float4> _r = ResourceDescriptorHeap[g_outTexIndex]; return _r; }
#define g_outTex Get_outTex()

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
