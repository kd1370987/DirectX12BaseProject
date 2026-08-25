//==========================================================================================
//
// ToneMapShader
//
// HDR のまま組み上げてきた最終カラー(AfterTAAColor)を、表示できる 0〜1 の範囲へ
// 落とし込んで FinalColor へ書き出す。
//
// どの曲線で落とすかは定数バッファで受け取る。絵作りの好みで切り替えたいものなので、
// シェーダーを差し替えるのではなくオプション(グラフィックス設定)から選べるようにしてある。
//
// このパスの出力が「最終テクスチャ」で、バックバッファへ載せるのは
// このあとの CopyToBackBufferPass の仕事。ここでは提示のことは考えない。
//
// 1:1 解像度なのでフィルタは不要。Load で直接読む。
//
//==========================================================================================
#include "../../../Common/RootParameters/ToneMapOptionData.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b14)        トーンマップ設定
//   1 : SRVテーブル(t0) 入力カラー(AfterTAAColor / HDR)
//   2 : UAVテーブル(u0) 出力カラー(FinalColor / LDR)
//==========================================================================================
#define TONEMAP_ROOT_SIG \
"RootFlags(0)," \
"CBV(b14, visibility = SHADER_VISIBILITY_ALL)," \
"DescriptorTable(SRV(t0, numDescriptors=1))," \
"DescriptorTable(UAV(u0, numDescriptors=1))"

cbuffer CBToneMapOption : register(b14)
{
	ToneMapOptionData g_toneMap;
}

Texture2D<float4>   g_input  : register(t0);	// トーンマップ前のHDRカラー
RWTexture2D<float4> g_output : register(u0);	// 最終カラー

//------------------------------------------------------------------------------------------
// ACESフィルミック
//
// ハイライトを滑らかに圧縮する。そのぶん彩度や色味は変わって見える
//------------------------------------------------------------------------------------------
float3 ACESFilm(float3 a_color)
{
	const float _a = 2.51f;
	const float _b = 0.03f;
	const float _c = 2.43f;
	const float _d = 0.59f;
	const float _e = 0.14f;
	return saturate((a_color * (_a * a_color + _b)) / (a_color * (_c * a_color + _d) + _e));
}

//------------------------------------------------------------------------------------------
// Reinhard
//
// 白があまり白くならず、全体的に少し暗く落ち着いた絵になる
//------------------------------------------------------------------------------------------
float3 Reinhard(float3 a_color)
{
	return a_color / (1.0f + a_color);
}

//------------------------------------------------------------------------------------------
// Reinhard(白点指定)
//
// 「この明るさを白として扱う」を決められるぶん、素の Reinhard より白が伸びる
//------------------------------------------------------------------------------------------
float3 ReinhardExtended(float3 a_color, float a_whitePoint)
{
	const float _white = max(a_whitePoint, 1e-4f);
	return a_color * (1.0f + a_color / (_white * _white)) / (1.0f + a_color);
}

//------------------------------------------------------------------------------------------
// Uncharted2 フィルミック
//
// ハイライトが自然で、強いブルームと相性がいい。
// 曲線そのものは 1 に収束しないので、白点での値で割って正規化する
//------------------------------------------------------------------------------------------
float3 Uncharted2Curve(float3 a_color)
{
	const float _a = 0.15f;
	const float _b = 0.50f;
	const float _c = 0.10f;
	const float _d = 0.20f;
	const float _e = 0.02f;
	const float _f = 0.30f;

	return ((a_color * (_a * a_color + _c * _b) + _d * _e) / (a_color * (_a * a_color + _b) + _d * _f)) - _e / _f;
}

float3 Uncharted2(float3 a_color, float a_whitePoint)
{
	const float3 _white = Uncharted2Curve(max(a_whitePoint, 1e-4f).xxx);
	return saturate(Uncharted2Curve(a_color) / max(_white, 1e-4f));
}

//------------------------------------------------------------------------------------------
// 種類で振り分ける
//------------------------------------------------------------------------------------------
float3 ApplyToneMap(float3 a_color)
{
	switch (g_toneMap.type)
	{
	case TONEMAP_TYPE_ACES:					return ACESFilm(a_color);
	case TONEMAP_TYPE_REINHARD:				return Reinhard(a_color);
	case TONEMAP_TYPE_REINHARD_EXTENDED:	return ReinhardExtended(a_color, g_toneMap.whitePoint);
	case TONEMAP_TYPE_UNCHARTED2:			return Uncharted2(a_color, g_toneMap.whitePoint);
	default:								return saturate(a_color);	// TONEMAP_TYPE_NONE
	}
}

[RootSignature(TONEMAP_ROOT_SIG)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// 出力サイズを取得して画面外スレッドを早期リターン
	uint _width, _height;
	g_output.GetDimensions(_width, _height);
	if (DTid.x >= _width || DTid.y >= _height) return;

	const int2 _coord = int2(DTid.xy);

	// 露出 → トーンマップ の順に掛ける。
	// 逆にすると 0〜1 へ落としたあとで持ち上げることになり、
	// 圧縮したハイライトがそのまま白飛びに戻ってしまう
	float3 _color = g_input.Load(int3(_coord, 0)).rgb;
	_color *= g_toneMap.exposure;
	_color = ApplyToneMap(_color);

	g_output[_coord] = float4(_color, 1.0f);
}
