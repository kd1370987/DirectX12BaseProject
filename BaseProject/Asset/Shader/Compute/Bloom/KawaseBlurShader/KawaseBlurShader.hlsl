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
#include "../../../Source/RootSignatureLayout.hlsli"

// ルートシグネチャデータ
#define KAWASE_BLUR_RS \
"RootFlags(0)," \
"DescriptorTable(SRV(t0, numDescriptors=4)), " \
"DescriptorTable(UAV(u0, numDescriptors=1)), " \
RS_STATIC_SAMPLER_CLAMP

// ボケ画像（それぞれ解像度が違う）
Texture2D<float4> g_bokenTex_0 : register(t0);	// 1/2
Texture2D<float4> g_bokenTex_1 : register(t1);	// 1/4
Texture2D<float4> g_bokenTex_2 : register(t2);	// 1/8
Texture2D<float4> g_bokenTex_3 : register(t3);	// 1/16

// 出力
RWTexture2D<float4> g_outTex : register(u0);

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
