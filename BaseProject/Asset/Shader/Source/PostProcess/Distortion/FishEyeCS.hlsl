//==========================================================================================
//
// FishEyeCS
//
// メインカラーを、center を中心に外へ向かって引き伸ばす。
// 魚眼レンズ越しに覗いたような歪みを出す画面効果。
//
//   注目画素のUVを中心から見た方向ベクトルに直し、
//   中心からの距離 r に応じて伸ばした先(center + dir * (1 + strength * r * r))を拾う。
//
//   ずらし量が r の二乗に比例するので、中心付近はほとんど動かず、
//   画面の隅へ行くほど大きく膨らむ = レンズらしい歪み方になる。
//
// 距離の計算だけアスペクト比を掛けて正す。これをしないと横長の画面で
// 「歪みの等高線」が楕円になり、上下だけ強く効いて見える。
//
// 無効時はそのまま素通しするので、絵は変わらない。
//
//==========================================================================================
#include "../../../Common/RootSignatureLayout.hlsli"

#include "../../../Common/RootParameters/FishEyeOptionData.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b16)           魚眼レンズの設定
//   1 : SRVテーブル(t0)    メインカラー
//   2 : UAVテーブル(u0)    出力カラー
//
// サンプラーは CLAMP。歪ませたUVは画面外を指すことがあるので、
// WRAP のままだと反対側の色が回り込んで端に別の絵が滲む
//==========================================================================================
#define FISH_EYE_RS \
"RootFlags(0)," \
"CBV(b16, visibility = SHADER_VISIBILITY_ALL)," \
"DescriptorTable(SRV(t0, numDescriptors=1)), " \
"DescriptorTable(UAV(u0, numDescriptors=1)), " \
RS_STATIC_SAMPLER_CLAMP

cbuffer CBFishEyeOption : register(b16)
{
	FishEyeOptionData g_fishEye;
}

// 入力
Texture2D<float4> g_colorTex : register(t0);	// メインカラー

// 出力
RWTexture2D<float4> g_outTex : register(u0);

// サンプラー
SamplerState g_samp : register(s0);

[RootSignature(FISH_EYE_RS)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// 出力画像の解像度を取得
	uint _width, _height;
	g_outTex.GetDimensions(_width, _height);

	// 画面外チェック
	if (DTid.x >= _width || DTid.y >= _height) return;

	int2 _coord = int2(DTid.xy);

	// 無効ならそのまま通す
	if (g_fishEye.enable == 0)
	{
		g_outTex[_coord] = g_colorTex.Load(int3(_coord, 0));
		return;
	}

	// 画素の中心を指すUV(+0.5 を足さないと半画素ずれる)
	float2 _uv = (DTid.xy + 0.5f) / float2(_width, _height);

	// 中心からどちらへどれだけ離れているか
	float2 _direction = _uv - g_fishEye.center;

	// 距離の計算だけアスペクト比を掛けて正す。
	// 補正した方向ベクトルのまま引き伸ばすとUVが横に潰れるので、
	// 長さを測るのに使うのは別に持っておく
	float _aspect = (float)_width / (float)_height;
	float2 _aspectDir = float2(_direction.x * _aspect, _direction.y);

	// 中心からの距離
	float _r = length(_aspectDir);

	// 中心から遠いほど強く伸ばす(r の二乗に比例)
	float _scale = 1.0f + g_fishEye.strength * _r * _r;

	// 歪ませた先のUV
	float2 _distortedUV = g_fishEye.center + _direction * _scale;

	// 画面の外を指したところは黒く落とす。
	// レンズの外側(絵が無いところ)なので、CLAMP で端の色を引き伸ばすと
	// 四隅に縞が伸びたような汚れ方になる
	if (any(_distortedUV < 0.0f) || any(_distortedUV > 1.0f))
	{
		g_outTex[_coord] = float4(0, 0, 0, 1);
		return;
	}

	// 出力
	g_outTex[_coord] = g_colorTex.SampleLevel(g_samp, _distortedUV, 0);
}
