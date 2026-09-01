//==========================================================================================
// BlendShader
//
// メインとなる画像に重ねる
//==========================================================================================
#include "../../../Common/RootSignatureLayout.hlsli"

//==========================================================================================
// ルートパラメーター
//==========================================================================================
#define BLEND_ROOT_SIG \
"RootFlags(0)," \
"DescriptorTable(SRV(t0, numDescriptors=2)), " \
"DescriptorTable(UAV(u0, numDescriptors=1)), " \
RS_STATIC_SAMPLER


// 入力
Texture2D<float4> g_colorTex : register(t0); // メインカラー
Texture2D<float4> g_blendTex : register(t1); // 重ねる画像

// 出力
RWTexture2D<float4> g_output : register(u0);

// サンプラー
SamplerState g_samp : register(s0);

[RootSignature(BLEND_ROOT_SIG)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// 画像の解像度を取得
	uint _width, _height;
	g_output.GetDimensions(_width, _height);

	// 画面外ならリターン
	if (DTid.x >= _width || DTid.y >= _height)
		return;

	int2 _coord = int2(DTid.xy);
	float4 _centerColor = g_colorTex.Load(int3(_coord, 0));
	float4 _centerBlendColor = g_blendTex.Load(int3(_coord, 0));

	// 重ねる画像は「透明な黒で消した板へ SRC_ALPHA / INV_SRC_ALPHA で描いたもの」なので、
	// RGB にはすでにアルファが掛かっている(プリマルチプライドアルファ)。
	//
	//   板の RGB = 描いた色 * a
	//   板の A   = a
	//
	// ここで lerp(下, 板, a) にすると a を二重に掛けることになり、
	// 半透明のUIが本来より暗く沈む。掛け算は板を描いた時点で済んでいるので、
	// 下の絵を (1 - a) で空けて、そこへ板をそのまま足すのが正しい
	float3 _outRGB = _centerColor.rgb * (1.0f - _centerBlendColor.a) + _centerBlendColor.rgb;

	g_output[_coord] = float4(_outRGB, 1.0f);
}
