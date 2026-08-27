//==========================================================================================
//
// RadialBlurCS
//
// メインカラーを、blurCenter から放射状に引きずってボカす。
// 加速時のスピード感や被弾時の衝撃を出す画面効果。
//
//   注目画素から中心へ向かって少しずつ戻りながらサンプリングし、平均を取る。
//   引きずる長さは「中心からの距離 × strength」なので、
//   中心付近はほとんど動かず、画面端へ行くほど強く流れる。
//
// 中心付近(radius の内側)はボカさない。ここを残さないと、
// 注視している真ん中まで流れて何も見えなくなる。
//
// 無効時はそのまま素通しするので、絵は変わらない。
//
//==========================================================================================
#include "../../../Common/RootSignatureLayout.hlsli"

#include "../../../Common/RootParameters/RadialBlurOptionData.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b16)           ラジアルブラーの設定
//   1 : SRVテーブル(t0)    メインカラー
//   2 : UAVテーブル(u0)    出力カラー
//
// サンプラーは CLAMP。放射状に引きずると必ず画面外を舐めるので、
// WRAP のままだと反対側の色が回り込んで端に別の絵が滲む
//==========================================================================================
#define RADIAL_BLUR_RS \
"RootFlags(0)," \
"CBV(b16, visibility = SHADER_VISIBILITY_ALL)," \
"DescriptorTable(SRV(t0, numDescriptors=1)), " \
"DescriptorTable(UAV(u0, numDescriptors=1)), " \
RS_STATIC_SAMPLER_CLAMP

cbuffer CBRadialBlurOption : register(b16)
{
	RadialBlurOptionData g_radial;
}

// 入力
Texture2D<float4> g_colorTex : register(t0);	// メインカラー

// 出力
RWTexture2D<float4> g_outTex : register(u0);

// サンプラー
SamplerState g_samp : register(s0);

[RootSignature(RADIAL_BLUR_RS)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// 出力画像の解像度を取得
	uint _width, _height;
	g_outTex.GetDimensions(_width, _height);

	// 画面外チェック
	if (DTid.x >= _width || DTid.y >= _height) return;

	int2 _coord = int2(DTid.xy);
	float4 _centerColor = g_colorTex.Load(int3(_coord, 0));

	// 無効ならそのまま通す
	if (g_radial.enable == 0)
	{
		g_outTex[_coord] = _centerColor;
		return;
	}

	// 画素の中心を指すUV(+0.5 を足さないと半画素ずれる)
	float2 _uv = (DTid.xy + 0.5f) / float2(_width, _height);

	// 中心からどれだけ離れているか
	float2 _direction = _uv - g_radial.blurCenter;
	float _distanceFromCenter = length(_direction);

	// 中心付近はブラーしない。radius から先が falloff の傾きで 0→1 へ立ち上がる
	float _blurMask = saturate((_distanceFromCenter - g_radial.radius) * g_radial.falloff);

	// 効かない画素はサンプリングごと省く
	if (_blurMask <= 0.0f)
	{
		g_outTex[_coord] = _centerColor;
		return;
	}

	// 1 サンプルも回らない設定でも 0 除算しない
	int _sampleCount = max(g_radial.sampleCount, 1);

	// 今フレームこの画素が引きずる量(UV)
	float2 _blurOffset = _direction * g_radial.strength * _blurMask;

	float4 _color = 0.0f;

	[loop]
	for (int _i = 0; _i < _sampleCount; ++_i)
	{
		// 0～1 を二乗して、注目画素の近くほどサンプルを密にする。
		// 等間隔だと引きずった先が同じ濃さで残って二重像に見える
		float _t = (float)_i / max((float)_sampleCount - 1.0f, 1.0f);
		_t *= _t;

		// 中心方向へ引っ張る(_uv + _blurOffset にすると外へ流れる形になる)
		float2 _sampleUV = _uv - _blurOffset * _t;

		// 画面外はサンプラーの CLAMP が端の色で止めてくれる
		_color += g_colorTex.SampleLevel(g_samp, _sampleUV, 0);
	}

	// 出力
	g_outTex[_coord] = _color / (float)_sampleCount;
}
