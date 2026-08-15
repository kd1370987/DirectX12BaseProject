//==========================================================================================
//
// KawaseBlurShader
//
// 川瀬式ブルームの合流点。
// 1/2・1/4・1/8・1/16 まで縮小しながらガウシアンブラーを掛け、そこから元のサイズまで
// 引き伸ばし直した4枚を1枚にまとめる。
//
// 縮小率が違うぶん、ボケの広がりも 4段階で変わっている。それを重ねることで、
// 「芯は明るく、外へ行くほどゆるく広がる」ブルーム特有の減衰が1枚で作れる。
// 単発の大きいブラーで同じ広がりを出そうとするとタップ数が跳ね上がるので、
// 縮小バッファを積む方が圧倒的に安い。
//
// 4枚とも入力の時点で元のサイズまで拡大済みなので、ここではUVを計算せず同じ座標を直接読む。
// 総量が4倍にならないよう平均でまとめ、強さは合成パス側の intensity で調整する。
//
//==========================================================================================
#include "../../../Source/RootSignatureLayout.hlsli"

// ルートシグネチャデータ
#define KAWASE_BLUR_RS \
"RootFlags(0)," \
"DescriptorTable(SRV(t0, numDescriptors=4)), " \
"DescriptorTable(UAV(u0, numDescriptors=1))"

// ボケ画像（すべて出力と同じ解像度まで拡大済み）
Texture2D<float4> g_bokenTex_0 : register(t0);	// 1/2  から拡大
Texture2D<float4> g_bokenTex_1 : register(t1);	// 1/4  から拡大
Texture2D<float4> g_bokenTex_2 : register(t2);	// 1/8  から拡大
Texture2D<float4> g_bokenTex_3 : register(t3);	// 1/16 から拡大

// 出力
RWTexture2D<float4> g_outTex : register(u0);

[RootSignature(KAWASE_BLUR_RS)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// 画像の縦横サイズを取得
	uint _width, _height;
	g_outTex.GetDimensions(_width, _height);

	// 画面外ならリターン
	if (DTid.x >= _width || DTid.y >= _height) return;

	float4 _outColor = g_bokenTex_0[DTid.xy].rgba;
	_outColor += g_bokenTex_1[DTid.xy].rgba;
	_outColor += g_bokenTex_2[DTid.xy].rgba;
	_outColor += g_bokenTex_3[DTid.xy].rgba;

	_outColor /= 4;
	_outColor.a = 1.0f;

	g_outTex[DTid.xy] = _outColor;
}
