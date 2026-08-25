#include "../../../../Common/Math/CalcNormal.hlsli"
#include "../../../../Common/RootSignatureLayout.hlsli"
#include "../../../../Common/RootParameters/CameraData.hlsli"
#include "../../../../Common/RootParameters/DenoiseSetting.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)            カメラ
//   1 : CBV(b1)            デノイズ設定(A-Trous のステップごとに書き換える)
//   2 : SRVテーブル(t0-t2) GI(ハーフ) + 深度・法線(フル)
//   3 : UAVテーブル(u0)    出力GI(ハーフ)
//==========================================================================================
#define GISPATIALDENOISE_ROOT_SIG \
"RootFlags(0), " \
"CBV(b0, visibility = SHADER_VISIBILITY_ALL)," \
"CBV(b1)," \
"DescriptorTable(SRV(t0, numDescriptors=3)),"\
"DescriptorTable(UAV(u0, numDescriptors=1)),"\
RS_STATIC_SAMPLER

cbuffer CBCamera : register(b0)
{
	CameraData g_camera;
}

cbuffer CBDenoiseSettings : register(b1)
{
	SpatialDenoiseSetting g_denoiseSettings;
}

// 入力
Texture2D<float4> g_GITex : register(t0);		// 時間デノイズされたGI	(ハーフ解像度)
Texture2D<float4> g_depthTex : register(t1);	// 現在深度				(フル解像度)
Texture2D<float4> g_normalTex : register(t2);	// 現在法線				(フル解像度)

// 出力
RWTexture2D<float4> g_outputGI : register(u0); // 結果書き込み用			(ハーフ解像度)

// サンプラー
SamplerState g_smp : register(s0);

// 5x5 A-Trous カーネル用のB3スプライン重み（1次元）
// 二次元展開したときの中心(0)が 6/16, 前後が 4/16,1/16
static const float g_kernel[5] = {
	1.0f / 16.0f,
	4.0f / 16.0f,
	6.0f / 16.0f,
	4.0f / 16.0f,
	1.0f / 16.0f
};

// ---------------------------------------------------------------------------
// GIはハーフ解像度、深度・法線はGBuffer由来なのでフル解像度。
// レイ生成シェーダーが 2x2 ブロックの左上テクセルを見ているので、ここも合わせる
// ---------------------------------------------------------------------------
int2 HalfToFull(int2 a_halfCoord)
{
	return a_halfCoord * 2;
}

// フル解像度のピクセル座標とデバイス深度からワールド座標を復元する
float3 ReconstructWorldPos(int2 a_fullCoord, float2 a_fullDim, float a_depth)
{
	float2 _uv = (float2(a_fullCoord) + 0.5f) / a_fullDim;
	float4 _clip = float4(_uv.x * 2.0f - 1.0f, 1.0f - _uv.y * 2.0f, a_depth, 1.0f);
	float4 _world = mul(_clip, g_camera.invViewProj);
	return _world.xyz / _world.w;
}

// 輝度
float Luminance(float3 a_color)
{
	return dot(a_color, float3(0.2126f, 0.7152f, 0.0722f));
}

// ルートシグネチャセット
[RootSignature(GISPATIALDENOISE_ROOT_SIG)]

[numthreads(8, 8, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
	// 画面サイズを取得（出力＝ハーフ解像度）
	uint _width, _height;
	g_outputGI.GetDimensions(_width,_height);

	// 画面外スレッドの早期抜出
	if (DTid.x >= _width || DTid.y >= _height) return;

	int2 _centerCoord = int2(DTid.xy);
	float2 _fullDim = float2(_width, _height) * 2.0f;
	int2 _centerFullCoord = HalfToFull(_centerCoord);

	// 注目画素（中心）の情報を取得
	float4 _centerColor = g_GITex.Load(int3(_centerCoord,0));
	float _centerDepth = g_depthTex.Load(int3(_centerFullCoord, 0)).r;
	float3 _centerNormal = DecsodeNormal(g_normalTex.Load(int3(_centerFullCoord, 0)).xy);

	// 中心のワールド座標とカメラからの距離
	// （エッジ判定のしきい値を距離に比例させるために使う）
	float3 _centerWorldPos = ReconstructWorldPos(_centerFullCoord, _fullDim, _centerDepth);
	float _centerDist = max(length(g_camera.cameraPos.xyz - _centerWorldPos), 1e-3f);

	// ---- 局所的な輝度の分散を求める ----
	// 生のRGB距離で色ウェイトを作ると「ノイズそのものをエッジとして保護」してしまい、
	// 高周波ノイズが低周波の斑点として残る(ぼやけた円状のノイズ跡の正体)。
	// ノイズの大きさ(標準偏差)でしきい値を正規化することで、
	// ノイズは平均化しつつ本物のエッジだけを残す。
	float _lumSum = 0.0f;
	float _lumSqSum = 0.0f;
	[unroll]
	for (int _vy = -1; _vy <= 1; ++_vy)
	{
		[unroll]
		for (int _vx = -1; _vx <= 1; ++_vx)
		{
			int2 _c = clamp(_centerCoord + int2(_vx, _vy), int2(0, 0), int2(_width - 1, _height - 1));
			float _l = Luminance(g_GITex.Load(int3(_c, 0)).rgb);
			_lumSum += _l;
			_lumSqSum += _l * _l;
		}
	}
	float _lumMean = _lumSum / 9.0f;
	float _lumSigma = sqrt(max(_lumSqSum / 9.0f - _lumMean * _lumMean, 0.0f));

	// 色ウェイトの分母。分散が0(平坦)でもゼロ除算しないように下駄を履かせる
	float _colorDenom = g_denoiseSettings.phiColor * _lumSigma + 1e-3f;

	// 深度ウェイトの許容量。
	// ・カメラからの距離に比例（遠景ほど1ピクセルが覆うワールド距離が大きい）
	// ・ステップサイズに比例（カーネルが広がるほどタップ間の距離も広がる）
	//   これをやらないと後半の広いパスで全サンプルが弾かれ、フィルタが効かなくなる
	float _depthDenom = max(
		g_denoiseSettings.phiDepth * _centerDist * (float) g_denoiseSettings.stepSize,
		1e-4f);

	float4 _sumColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float _sumWeight = 0.0f;

	// 5x5の近傍ピクセルをループ
	// A-Trousアルゴリズムを使う
	// 定数バッファでもらい受けたステップサイズに応じて少しずつ離れた場所をサンプリングする
	[unroll]
	for (int _y = -2; _y <= 2; ++_y)
	{
		[unroll]
		for (int _x = -2; _x <= 2; ++_x)
		{
			// サンプリング座標を計算（ステップサイズに応じて外側に広がる）
			int2 _sampleCoord = _centerCoord + int2(_x, _y) * g_denoiseSettings.stepSize;

			// 画面クランプ
			_sampleCoord = clamp(_sampleCoord, int2(0, 0), int2(_width - 1, _height - 1));

			// サンプル画素の情報を取得
			// DepthとNormalはフル解像度から引っ張る
			int2 _sampleFullCoord = HalfToFull(_sampleCoord);
			float4 _sampleColor	 = g_GITex.Load(int3(_sampleCoord,0));
			float _sampleDepth = g_depthTex.Load(int3(_sampleFullCoord, 0)).r;
			float3 _sampleNormal = DecsodeNormal(g_normalTex.Load(int3(_sampleFullCoord, 0)).xy);

			// ---- ベースとなるフィルターの重み : B3 Spline ----
			float _filterWeight = g_kernel[_x + 2] * g_kernel[_y + 2];

			// ---- 深度エッジウェイト計算 ----
			// 生の非線形デバイス深度の差は距離によって精度が偏り、
			// 近景では常に弾かれ遠景では常に通過してしまうので、
			// ワールド座標を復元して「中心の接平面からどれだけ浮いているか」で評価する。
			// 同一平面上なら傾いていても距離が0になるので、斜めの床や壁でもきちんと均せる。
			float3 _sampleWorldPos = ReconstructWorldPos(_sampleFullCoord, _fullDim, _sampleDepth);
			float _planeDist = abs(dot(_sampleWorldPos - _centerWorldPos, _centerNormal));
			float _wDepth = exp(-_planeDist / _depthDenom);

			// ---- 法線エッジウェイトの計算 ----
			// 内積(cosΘ)をベースに、角度のずれが急かどうか評価
			float _normalDot = max(0.0f, dot(_centerNormal, _sampleNormal));
			float _wNormal = pow(_normalDot, g_denoiseSettings.phiNormal);

			// ---- カラーウェイトの計算（輝度差をノイズの大きさで正規化） ----
			float _lumDiff = abs(Luminance(_centerColor.rgb) - Luminance(_sampleColor.rgb));
			float _wColor = exp(-_lumDiff / _colorDenom);

			// すべての重みを乗算
			float _finalWeight = _filterWeight * _wDepth * _wNormal * _wColor;

			// ブレンド計算
			_sumColor += _sampleColor * _finalWeight;
			_sumWeight += _finalWeight;
		}
	}

	// 重みの合計で正規化して出力（ゼロ除算対策）
	if (_sumWeight > 0.0f)
	{
		g_outputGI[ _centerCoord] = _sumColor / _sumWeight;
	}
	else
	{
		g_outputGI[_centerCoord] = _centerColor;
	}
}
