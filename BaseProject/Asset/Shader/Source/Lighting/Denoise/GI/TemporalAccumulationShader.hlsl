#include "../../../../Common/Math/CalcNormal.hlsli"
#include "../../../../Common/RootSignatureLayout.hlsli"
#include "../../../../Common/RootParameters/CameraData.hlsli"
#include "../../../../Common/RootParameters/DenoiseSetting.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)            カメラ
//   1 : CBV(b1)            時間累積の設定
//   2 : SRVテーブル(t0-t6) 現在のGI + 速度 + 履歴 + 現在/過去の深度・法線
//   3 : UAVテーブル(u0)    出力GI(ハーフ)
//
// 履歴のサンプラーは CLAMP。WRAP のままだと画面端で反対側の色が回り込む
//==========================================================================================
#define TEMPORALACCUMULATION_ROOT_SIG \
"RootFlags(0), " \
"CBV(b0, visibility = SHADER_VISIBILITY_ALL)," \
"CBV(b1),"\
"DescriptorTable(SRV(t0, numDescriptors=7)),"\
"DescriptorTable(UAV(u0, numDescriptors=1)),"\
RS_STATIC_SAMPLER_CLAMP

cbuffer CBCamera : register(b0)
{
	CameraData g_camera;
}

cbuffer CBGITAOption : register(b1)
{
	TemporalAccumulationSetting g_option;
}

// 画面UV と デバイス深度からビュー空間座標を復元する
float3 ReconstructViewPos(float2 a_uv, float a_depth)
{
	float4 _clip = float4(a_uv.x * 2.0f - 1.0f, 1.0f - a_uv.y * 2.0f, a_depth, 1.0f);
	float4 _view = mul(_clip, g_camera.invProj);
	return _view.xyz / _view.w;
}

// 入力
Texture2D<float4> g_currentGITex : register(t0);	// 現在のGI			(ハーフ解像度)
Texture2D<float4> g_velocityTex : register(t1);		// モーションベクター	(フル解像度)
Texture2D<float4> g_historyGITex : register(t2);	// 前フレームのGI		(ハーフ解像度)
Texture2D<float4> g_depthTex : register(t3);		// 現在深度			(フル解像度)
Texture2D<float4> g_normalTex : register(t4);		// 現在法線			(フル解像度)
Texture2D<float4> g_prevDepthTex : register(t5);	// 過去深度			(フル解像度)
Texture2D<float4> g_prevNormalTex : register(t6);	// 過去法線			(フル解像度)

// 出力
RWTexture2D<float4> g_outputGI : register(u0);		// 結果書き込み用		(ハーフ解像度)

// サンプラー
SamplerState g_smp : register(s0);

// ---------------------------------------------------------------------------
// 解像度について
//
// このパスはGI(ハーフ解像度)を処理するが、深度・法線・モーションベクターは
// GBuffer由来なのでフル解像度である。
// レイ生成シェーダー(RaytracingGI.hlsl)は 2x2 ブロックの左上テクセル
// (fullResId = id * 2) のGBufferを見てレイを飛ばしているので、
// ここでも必ず同じテクセルを見ないと「別のピクセルの深度/法線/速度」で
// 再投影と棄却判定を行うことになる。
// ※以前はここが Load(DTid.xy) になっており、画面左上1/4のGBufferを
//   画面全体に使ってしまっていた(履歴が常に壊れる原因)。
// ---------------------------------------------------------------------------
int2 HalfToFull(int2 a_halfCoord)
{
	return a_halfCoord * 2;
}

// ルートシグネチャセット
[RootSignature(TEMPORALACCUMULATION_ROOT_SIG)]

// ---- DTid ----
// ディスパッチ全体でこのスレッドは何番目かの変数
// このシェーダーではテクスチャを処理するため今のピクセル座標となる
[numthreads(8, 8, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
	// 画像の解像度を取得（出力＝ハーフ解像度）
	uint _width, _height;
	g_outputGI.GetDimensions(_width, _height);
	int2 _centerCoord = int2(DTid.xy);

	if (DTid.x >= _width || DTid.y >= _height) return;

	// ハーフ／フルそれぞれの解像度
	float2 _halfDim = float2(_width, _height);
	float2 _fullDim = _halfDim * 2.0f;

	// このハーフ解像度ピクセルが担当するフル解像度テクセル
	int2 _fullCoord = HalfToFull(_centerCoord);

	// 画面UV（フル解像度基準。モーションベクターと同じ空間）
	float2 _uv = (float2(_fullCoord) + 0.5f) / _fullDim;

	// 現在の情報を取得
	float4 _currentGI = g_currentGITex.Load(int3(_centerCoord, 0));
	float3 _currentNormal = DecsodeNormal(g_normalTex.Load(int3(_fullCoord, 0)).rg);
	float _currentDepth = g_depthTex.Load(int3(_fullCoord, 0)).r;

	// -------------------------------------------------------------------------------
	// 3x3の近傍ピクセルをループ
	// 平均(m1)と二乗平均(m2)から分散を求めて統計的にクランプ範囲を決める(Variance Clipping)
	// 同時に一番手前のピクセルを探して Velocity Dilation に使う

	float3 _m1 = float3(0, 0, 0);	// 色の合計
	float3 _m2 = float3(0, 0, 0);	// 色の二乗の合計

	// Velocity Dilation用（一番手前のピクセルのモーションベクターを採用する）
	float _closestDepth = 1e10f;
	int2 _closestFullCoord = _fullCoord;

	[unroll]
	for (int _y = -1; _y <= 1; ++_y)
	{
		[unroll]
		for (int _x = -1; _x <= 1; ++_x)
		{
			// サンプリング座標を取得
			int2 _sampleCoord = _centerCoord + int2(_x, _y);
			_sampleCoord = clamp(_sampleCoord, int2(0, 0), int2(_width - 1, _height - 1)); // 画面クランプ

			// サンプル画素を取得
			float3 color = g_currentGITex.Load(int3(_sampleCoord, 0)).rgb;
			_m1 += color;
			_m2 += color * color;

			// もっと手前のピクセルを探す(Velocity Dilation)
			// 深度はフル解像度なので対応するテクセルへ変換してから引く
			int2 _sampleFullCoord = HalfToFull(_sampleCoord);
			float _d = g_depthTex.Load(int3(_sampleFullCoord, 0)).r;
			if (_d < _closestDepth)
			{
				_closestDepth = _d;
				_closestFullCoord = _sampleFullCoord;
			}
		}
	}

	// 平均(mu)と分散から標準偏差(sigma)を求める
	float3 _mu = _m1 / 9.0f;
	float3 _sigma = sqrt(abs(_m2 / 9.0f - _mu * _mu));

	// 係数ガンマ（小さいほどゴーストは減るがボケやすくなる）
	// GIは1sppで分散が大きいので影より広めに取る
	const float _gamma = 2.0f;
	float3 _minColor = _mu - _gamma * _sigma;
	float3 _maxColor = _mu + _gamma * _sigma;

	// モーションベクターから過去UVを取得
	// 中心ではなく一番手前にあるピクセルのVelocityを使って過去UVを計算する
	float2 _velocity = g_velocityTex.Load(int3(_closestFullCoord, 0)).xy;
	float2 _prevUV = _uv - _velocity;

	// 画面外チェック
	if (_prevUV.x < 0.0f || _prevUV.x > 1.0f || _prevUV.y < 0.0f || _prevUV.y > 1.0f)
	{
		// 画面外から入ってきた新しいピクセルは過去の履歴がないのでそのまま出力
		g_outputGI[_centerCoord] = _currentGI;
		return;
	}

	// ---- 画面UV → ハーフ解像度テクスチャのUV ----
	// 履歴はハーフ解像度で、テクセルkがフル解像度ピクセル2kを表す。
	// 画面UVをそのまま使うと半ピクセルずれた場所を引いてしまうので変換する。
	float2 _prevHalfTexel = (_prevUV * _fullDim - 0.5f) * 0.5f;
	float2 _prevHistoryUV = (_prevHalfTexel + 0.5f) / _halfDim;

	// 過去の履歴はサブピクセル精度のためバイリニアで取得する
	float4 _historyGI = g_historyGITex.SampleLevel(g_smp, _prevHistoryUV, 0);

	// ---- 棄却判定に使う過去深度/法線はポイントサンプルする ----
	// バイリニアだとエッジで前景と背景の深度/法線が混ざり、
	// ディスオクルージョン判定が「一致寄り」に引っ張られてすり抜ける(=ゴーストの一因)。
	int2 _prevFullCoord = int2(_prevUV * _fullDim);
	_prevFullCoord = clamp(_prevFullCoord, int2(0, 0), int2(_fullDim) - 1);
	float3 _prevNormal = DecsodeNormal(g_prevNormalTex.Load(int3(_prevFullCoord, 0)).rg);
	float _prevDepth = g_prevDepthTex.Load(int3(_prevFullCoord, 0)).r;

	// 統計的なクランプで履歴を現在の分布内に収める(Variance Clipping)
	_historyGI.rgb = clamp(_historyGI.rgb, _minColor, _maxColor);

	// -------------------------------------------------------------------------------
	// ゴースト対策（ディスオクルージョン判定）
	// これまで隠れていた背景や物体が新たに露出する現象を検出する
	bool _isValidHistory = true;

	// 法線の向きが違いすぎる場合は履歴を捨てる
	// phiNormal は dot積(最大1.0)と比較するしきい値。1を超える値を入れると
	// 常に不成立になりテンポラルデノイズが丸ごと無効化されるので注意
	if (dot(_currentNormal, _prevNormal) < g_option.phiNormal)
	{
		_isValidHistory = false;
	}

	// 位置(ビュー空間)がずれすぎる場合は別サーフェスとみなして履歴を捨てる。
	// 生の非線形デバイス深度の単純比較は距離によって精度が偏り(近景は常に棄却・
	// 遠景は常に通過)、輪郭ですり抜けるため、ビュー空間座標を復元して距離で比較し、
	// 深度に対する相対しきい値で判定する。
	float3 _curViewPos  = ReconstructViewPos(_uv, _currentDepth);
	float3 _prevViewPos = ReconstructViewPos(_prevUV, _prevDepth);
	float _posDist = length(_curViewPos - _prevViewPos);
	if (_posDist > g_option.phiDepth * max(abs(_curViewPos.z), 1e-3f))
	{
		_isValidHistory = false;
	}

	// -------------------------------------------------------------------------------
	// ブレンド処理
	float4 _finalGI = _currentGI;

	if(_isValidHistory)
	{
		// 履歴が有効ならブレンド
		// アルファが小さいほどノイズは消えるが、動いた時の残像が出やすくなる
		float _alpha = g_option.blendRate;
		_finalGI.rgb = lerp(_historyGI.rgb, _currentGI.rgb, _alpha);
	}

	// 結果を出力（RGBに色、AにHitDistance）
	// アルファ(HitDistance)は現在フレームの値をそのまま通す。
	// 距離を時間ブレンドしても意味のある値にならないため
	g_outputGI[_centerCoord] = float4(_finalGI.rgb, _currentGI.a);
}
