#include "../../../Common/Math/CalcNormal.hlsli"
#include "../../../Common/RootParameters/CameraData.hlsli"
#include "../../../Common/RootParameters/UpScaleSetting.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)         カメラ
//   1 : CBV(b1)         引き伸ばしの許容差
//   2 : SRVテーブル(t0) 低解像度カラー
//   3 : SRVテーブル(t1) 深度(フル)
//   4 : SRVテーブル(t2) 法線(フル)
//   5 : UAVテーブル(u0) 出力カラー(フル)
//==========================================================================================
#define UPSCALE_RS \
    "CBV(b0), " \
    "CBV(b1), " \
    "DescriptorTable(SRV(t0)), " \
    "DescriptorTable(SRV(t1)), " \
    "DescriptorTable(SRV(t2)), " \
    "DescriptorTable(UAV(u0))"

// 入力テクスチャ
Texture2D<float4> g_lowResColorTex : register(t0);	// レイトレ結果
Texture2D<float> g_fullResDepthTex : register(t1);	// 深度
Texture2D<float2> g_fullResNormalTex : register(t2);	// 法線

// 出力テクスチャ
RWTexture2D<float4> g_outputTex : register(u0);		// アップスケール結果

cbuffer CBCamera : register(b0)
{
	CameraData g_camera;
}

cbuffer CBUpScaleSetting : register(b1)
{
	UpScaleSetting g_upScale;
}

// フル解像度のピクセル座標とデバイス深度からワールド座標を復元する
float3 ReconstructWorldPos(int2 a_fullCoord, float2 a_fullDim, float a_depth)
{
	float2 _uv = (float2(a_fullCoord) + 0.5f) / a_fullDim;
	float4 _clip = float4(_uv.x * 2.0f - 1.0f, 1.0f - _uv.y * 2.0f, a_depth, 1.0f);
	float4 _world = mul(_clip, g_camera.invViewProj);
	return _world.xyz / _world.w;
}

// ==========================================
// コンピュートシェーダー本体
// ==========================================
[RootSignature(UPSCALE_RS)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// 現在処理しているフル解像度のピクセル座標
	uint2 _fullPos = DTid.xy;

	// 画像サイズを取得
	uint _fullWidth, _fullHeight;
	g_outputTex.GetDimensions(_fullWidth,_fullHeight);
	if (_fullPos.x >= _fullWidth || _fullPos.y >= _fullHeight) return;

	uint _lowWidth, _lowHeight;
	g_lowResColorTex.GetDimensions(_lowWidth, _lowHeight);

	// カレントピクセルの法線・深度を取得
	float _currentDepth = g_fullResDepthTex.Load(int3(_fullPos,0));
	float2 _enc = g_fullResNormalTex.Load(int3(_fullPos, 0)).rg;
	float3 _currentNormal = DecsodeNormal(_enc);					// 法線を復元

	// エッジ判定をワールド空間で行うための下ごしらえ。
	// 生の非線形デバイス深度の差で判定すると、近景では常に弾かれ遠景では常に通過して
	// しまい、事実上ただのバイリニア拡大になる(シルエットでGIがにじむ原因)
	float2 _fullDim = float2(_fullWidth, _fullHeight);
	float3 _currentWorldPos = ReconstructWorldPos(int2(_fullPos), _fullDim, _currentDepth);
	float _currentDist = max(length(g_camera.cameraPos.xyz - _currentWorldPos), 1e-3f);
	float _depthDenom = max(g_upScale.depthSigma * _currentDist, 1e-4f);

	// 対応する低解像度テクスチャの座標計算
	// ピクセル中心を考慮して低解像度での浮動小数点座標を求める
	float2 _lowResCoord = (_fullPos + 0.5f) / g_upScale.scaleRatio - 0.5f;

	// 周囲４ピクセルをとるためのベース座標（左上）と、補間ウェイト
	int2 _lowResBasePos = floor(_lowResCoord);
	float2 _fracPos = _lowResCoord - _lowResBasePos;

	float4 _totalColor = 0.0f;
	float _totalWeight = 0.0f;

	// 周囲４ピクセルをループしてブレンド計算
	[unroll]
	for (int _y = 0; _y < 2; ++_y)
	{
		[unroll]
		for (int _x = 0; _x < 2; ++_x)
		{
			// サンプリングする低解像度のピクセル座標 : 画面外に出ないようにクランプ
			int2 _sampleLowPos = _lowResBasePos + int2(_x,_y);
			_sampleLowPos = clamp(_sampleLowPos, int2(0, 0), int2(_lowWidth - 1, _lowHeight - 1));
			
			// フル解像度の法線 / 深度バッファから比較対象データをとるための座標。
			// この低解像度ピクセルが「どのフル解像度ピクセルを見て計算されたか」を求める。
			// レイ生成シェーダー(RaytracingGI.hlsl)は 2x2 ブロックの左上テクセル
			// (fullResId = id * 2) を見ているので、ここも同じテクセルを指さないと
			// ガイドが1テクセルずれ、シルエット際でにじみ・ちらつきになる。
			// ※以前は「+ scaleRatio * 0.5」でブロックの右下寄りを見ていた
			int2 _sampleFullPos = (int2) (_sampleLowPos * g_upScale.scaleRatio);
			_sampleFullPos = clamp(_sampleFullPos, int2(0, 0), int2(_fullWidth - 1, _fullHeight - 1));

			// サンプリングポイントの法線・深度を取得
			float _sampleDepth = g_fullResDepthTex.Load(int3(_sampleFullPos, 0));
			float2 _sampleEnc = g_fullResNormalTex.Load(int3(_sampleFullPos, 0)).rg;
			float3 _sampleNormal = DecsodeNormal(_sampleEnc);

			// ---- ウェイトの計算 ----
			// 空間ウェイト : バイリニア補間 = 距離が近いほど重い
			float _weightX = (_x == 0) ? (1.0f - _fracPos.x) : _fracPos.x;
			float _weightY = (_y == 0) ? (1.0f - _fracPos.y) : _fracPos.y;
			float _spatialWeight = _weightX * _weightY;

			// 深度ウェイト : 段差がある = 別オブジェクトなら重みを 0 に近づける
			// 「注目ピクセルの接平面からどれだけ浮いているか」で評価する。
			// 同一平面上なら傾いていても距離が0になるので、斜めの床や壁でも均せる
			float3 _sampleWorldPos = ReconstructWorldPos(_sampleFullPos, _fullDim, _sampleDepth);
			float _planeDist = abs(dot(_sampleWorldPos - _currentWorldPos, _currentNormal));
			float _depthWeight = exp(-_planeDist / _depthDenom);

			// 法線ウェイト : 向きが違う = 別オブジェクトや角なら重みを 0 に近づける
			float _normalDot = max(0.0f, dot(_currentNormal, _sampleNormal));
			float _normalWeight = pow(_normalDot, g_upScale.normalPower);

			// 最終的な重みを掛け合わせる
			float _finalWeight = _spatialWeight * _depthWeight * _normalWeight + 0.0001f;

			// 色と重みを加算
			float4 _sampleColor = g_lowResColorTex.Load(int3(_sampleLowPos,0));
			_totalColor += _sampleColor * _finalWeight;
			_totalWeight += _finalWeight;
		}
	}

	// 正規化してフル解像度バッファに書き込み
	g_outputTex[_fullPos] = _totalColor / _totalWeight;
	
}
