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
"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
"RootConstants(num32BitConstants=2, b100), " \
"RootConstants(num32BitConstants=1, b101), " \
RS_STATIC_SAMPLER


// 入力
// SRVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex0 : register(b100)
{
	uint g_colorTexIndex;
	uint g_blendTexIndex;
}

Texture2D<float4> Get_colorTex() { Texture2D<float4> _r = ResourceDescriptorHeap[g_colorTexIndex]; return _r; } // メインカラー
#define g_colorTex Get_colorTex()
Texture2D<float4> Get_blendTex() { Texture2D<float4> _r = ResourceDescriptorHeap[g_blendTexIndex]; return _r; } // 重ねる画像
#define g_blendTex Get_blendTex()

// 出力
// UAVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex1 : register(b101)
{
	uint g_outputIndex;
}

RWTexture2D<float4> Get_output() { RWTexture2D<float4> _r = ResourceDescriptorHeap[g_outputIndex]; return _r; }
#define g_output Get_output()

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
