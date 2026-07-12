#include "../../Source/CalcNormal.hlsli"

// ==========================================
// ルートシグネチャの定義
// ==========================================
#define UPSCALE_RS \
    "CBV(b0), " \
    "DescriptorTable(SRV(t0)), " \
    "DescriptorTable(SRV(t1)), " \
    "DescriptorTable(SRV(t2)), " \
    "DescriptorTable(UAV(u0))"

// ==========================================
// 必要なリソース
// ==========================================

// 入力テクスチャ
Texture2D<float4> g_lowResColorTex : register(t0);	// レイトレ結果
Texture2D<float> g_fullResDepthTex : register(t1);	// 深度
Texture2D<float2> g_fullResNormalTex : register(t2);	// 法線

// 出力テクスチャ
RWTexture2D<float4> g_outputTex : register(u0);		// アップスケール結果

// パラメータ
cbuffer CBUpscaleParams : register(b0)
{
	float g_scaleRatio;	// スケール倍率
	float g_depthSigma;	// 深度の許容差 : 小さいほど厳密にエッジで色を分ける
	float g_normalPower;	// 法線の許容差 : 大きいほど少しの角度の違いで色を分ける
	float g_padding;
};

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

	// 対応する低解像度テクスチャの座標計算
	// ピクセル中心を考慮して低解像度での浮動小数点座標を求める
	float2 _lowResCoord = (_fullPos + 0.5f) / g_scaleRatio - 0.5f;

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
			
			// 低解像度ピクセルの中心に当たる、フル解像度上での座標を推測
			// フル解像度の法線 / 深度バッファから、比較対象データをとってくるためのデータ
			int2 _sampleFullPos = (int2) (_sampleLowPos * g_scaleRatio + (g_scaleRatio * 0.5f));
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
			float _depthDiff = abs(_currentDepth - _sampleDepth);
			float _depthWeight = exp(-_depthDiff / g_depthSigma);

			// 法線ウェイト : 向きが違う = 別オブジェクトや角なら重みを 0 に近づける
			float _normalDot = max(0.0f, dot(_currentNormal, _sampleNormal));
			float _normalWeight = pow(_normalDot, g_normalPower);

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
