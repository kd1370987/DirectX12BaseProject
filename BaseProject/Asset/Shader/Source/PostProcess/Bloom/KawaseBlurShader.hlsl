//==========================================================================================
//
// KawaseBlurShader
//
// 川瀬式ブルームの合流点。
// 1/2・1/4・1/8・1/16 まで縮小しながらガウシアンブラーを掛けた4枚を1枚にまとめる。
//
// 縮小率が違うぶん、ボケの広がりも4段階で変わっている。それを重ねることで、
// 「芯は明るく、外へ行くほどゆるく広がる」ブルーム特有の減衰が1枚で作れる。
// 単発の大きいブラーで同じ広がりを出そうとするとタップ数が跳ね上がるので、
// 縮小バッファを積む方が圧倒的に安い。
//
// 4枚は解像度がバラバラのまま入ってくるので、UVでサンプリングして拡大を兼ねる。
// 元のサイズへ戻す専用パスは要らない（サンプラーのバイリニアが引き伸ばしてくれる上、
// 入力はすでにボケているので、拡大時の粗も出ない）。
//
// 総量が4倍にならないよう平均でまとめ、強さは合成パス側の intensity で調整する。
//
//==========================================================================================
#include "../../../Common/RootSignatureLayout.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : SRVの番号(t0-t3) 1/2・1/4・1/8・1/16 のボケ画像
//   1 : UAVの番号(u0)    まとめた結果
//==========================================================================================
#define KAWASE_BLUR_RS \
"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
"RootConstants(num32BitConstants=4, b100), " \
"RootConstants(num32BitConstants=1, b101), " \
RS_STATIC_SAMPLER_CLAMP

// ボケ画像（それぞれ解像度が違う）
// SRVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex0 : register(b100)
{
	uint g_bokenTex_0Index;
	uint g_bokenTex_1Index;
	uint g_bokenTex_2Index;
	uint g_bokenTex_3Index;
}

Texture2D<float4> Get_bokenTex_0() { Texture2D<float4> _r = ResourceDescriptorHeap[g_bokenTex_0Index]; return _r; }	// 1/2
#define g_bokenTex_0 Get_bokenTex_0()
Texture2D<float4> Get_bokenTex_1() { Texture2D<float4> _r = ResourceDescriptorHeap[g_bokenTex_1Index]; return _r; }	// 1/4
#define g_bokenTex_1 Get_bokenTex_1()
Texture2D<float4> Get_bokenTex_2() { Texture2D<float4> _r = ResourceDescriptorHeap[g_bokenTex_2Index]; return _r; }	// 1/8
#define g_bokenTex_2 Get_bokenTex_2()
Texture2D<float4> Get_bokenTex_3() { Texture2D<float4> _r = ResourceDescriptorHeap[g_bokenTex_3Index]; return _r; }	// 1/16
#define g_bokenTex_3 Get_bokenTex_3()

// 出力
// UAVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex1 : register(b101)
{
	uint g_outTexIndex;
}

RWTexture2D<float4> Get_outTex() { RWTexture2D<float4> _r = ResourceDescriptorHeap[g_outTexIndex]; return _r; }
#define g_outTex Get_outTex()

// サンプラー : 端をクランプする（WRAPだと画面端で反対側の光が回り込む）
SamplerState g_samp : register(s0);

[RootSignature(KAWASE_BLUR_RS)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// 画像の縦横サイズを取得
	uint _width, _height;
	g_outTex.GetDimensions(_width, _height);

	// 画面外ならリターン
	if (DTid.x >= _width || DTid.y >= _height) return;

	// 出力画素の中心に対応するUV。解像度が違っても同じUVで引ける
	float2 _uv = (DTid.xy + 0.5f) / float2(_width, _height);

	float4 _outColor = g_bokenTex_0.SampleLevel(g_samp, _uv, 0).rgba;
	_outColor += g_bokenTex_1.SampleLevel(g_samp, _uv, 0).rgba;
	_outColor += g_bokenTex_2.SampleLevel(g_samp, _uv, 0).rgba;
	_outColor += g_bokenTex_3.SampleLevel(g_samp, _uv, 0).rgba;

	_outColor /= 4;
	_outColor.a = 1.0f;

	g_outTex[DTid.xy] = _outColor;
}
