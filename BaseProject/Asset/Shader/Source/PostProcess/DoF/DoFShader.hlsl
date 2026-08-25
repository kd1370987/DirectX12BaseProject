//==========================================================================================
//
// DoFShader
//
// CoC(1チャンネル)を見てメインカラーをボカし、被写界深度を作る。
//
// 注目画素のボケ半径ぶんの円内をゴールデンアングルのスパイラルでサンプリングする
// 単純なギャザーぼかし。各サンプルは「そのサンプル自身のボケ円が注目画素まで届くか」で
// 重み付けするので、ピントの合った手前の物体に奥のボケが染み出しにくい。
//
//   ボケ半径(ピクセル) = abs(CoC) * maxBlurRadius
//
// ピント内(半径が1画素未満)と無効時はそのまま素通しする。
//
//==========================================================================================
#include "../../../Common/RootSignatureLayout.hlsli"

#include "../../../Common/RootParameters/DoFOptionData.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b12)           被写界深度の設定
//   1 : SRVテーブル(t0-t1) メインカラー + CoC
//   2 : UAVテーブル(u0)    出力カラー
//==========================================================================================
#define DOF_ROOT_SIG \
"RootFlags(0)," \
"CBV(b12, visibility = SHADER_VISIBILITY_ALL)," \
"DescriptorTable(SRV(t0, numDescriptors=2)), " \
"DescriptorTable(UAV(u0, numDescriptors=1)), " \
RS_STATIC_SAMPLER

cbuffer CBDoFOption : register(b12)
{
	DoFOptionData g_dof;
}

// 入力
Texture2D<float4> g_colorTex : register(t0);	// メインカラー
Texture2D<float>  g_cocTex   : register(t1);	// CoC

// 出力
RWTexture2D<float4> g_output : register(u0);

// サンプラー
SamplerState g_samp : register(s0);

// サンプル数。増やすほど滑らかになるが重くなる
#define DOF_TAP_COUNT 24

// 黄金角(ラジアン)。回すたびに前のサンプルと重ならない位置になる
#define GOLDEN_ANGLE 2.39996323f

[RootSignature(DOF_ROOT_SIG)]

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

	// 無効ならそのまま通す
	if (g_dof.enable == 0)
	{
		g_output[_coord] = _centerColor;
		return;
	}

	// 注目画素のボケ半径(ピクセル)
	float _centerCoC = g_cocTex.Load(int3(_coord, 0)).r;
	float _radius = abs(_centerCoC) * g_dof.maxBlurRadius;

	// ピントが合っている(1画素も広がらない)ならボカさない
	if (_radius < 0.5f)
	{
		g_output[_coord] = _centerColor;
		return;
	}

	float2 _texel = 1.0f / float2(_width, _height);
	float2 _uv = (DTid.xy + 0.5f) * _texel;

	// 中心画素は必ず重み1で入れる
	float3 _sum = _centerColor.rgb;
	float _weightSum = 1.0f;

	[loop]
	for (int _i = 0; _i < DOF_TAP_COUNT; ++_i)
	{
		// 円内へ均等に散らす(面積が均等になるよう半径は sqrt をとる)
		float _t = (_i + 0.5f) / DOF_TAP_COUNT;
		float _sampleDist = sqrt(_t) * _radius;
		float _angle = _i * GOLDEN_ANGLE;
		float2 _offset = float2(cos(_angle), sin(_angle)) * _sampleDist;

		// 画面外はクランプして端の色を拾う(共通サンプラーがWRAPなので自前で止める)
		float2 _sampleUV = saturate(_uv + _offset * _texel);

		float3 _sampleColor = g_colorTex.SampleLevel(g_samp, _sampleUV, 0).rgb;
		float _sampleCoC = g_cocTex.SampleLevel(g_samp, _sampleUV, 0).r;

		// そのサンプル自身のボケ円が注目画素まで届くか
		// (届かない＝ピントが合っている画素なら混ぜない)
		float _sampleRadius = abs(_sampleCoC) * g_dof.maxBlurRadius;
		float _weight = saturate(_sampleRadius - _sampleDist + 1.0f);

		_sum += _sampleColor * _weight;
		_weightSum += _weight;
	}

	g_output[_coord] = float4(_sum / _weightSum, _centerColor.a);
}
