// インクルードガード
#ifndef CB_TONEMAP_OPTION_HLSLI
#define CB_TONEMAP_OPTION_HLSLI

// トーンマップの種類
// ※ C++ 側 Engine::Option::GraphicsOptions::EToneMapType と数値を合わせること
#define TONEMAP_TYPE_NONE				0	// 掛けない(0〜1へ切り詰めるだけ)
#define TONEMAP_TYPE_ACES				1	// ACESフィルミック
#define TONEMAP_TYPE_REINHARD			2	// Reinhard
#define TONEMAP_TYPE_REINHARD_EXTENDED	3	// Reinhard(白点指定)
#define TONEMAP_TYPE_UNCHARTED2			4	// Uncharted2 フィルミック

// トーンマップの設定。
// OptionManager の ToneMapOption を C++ 側が詰めて、トーンマップパスへ送る。
// ※ C++ 側 ToneMapOptionData と並びを合わせること
struct ToneMapOptionData
{
	uint	type;			// 上の TONEMAP_TYPE_*
	float	exposure;		// トーンマップを掛ける前に乗せる露出倍率
	float	whitePoint;		// ReinhardExtended / Uncharted2 が使う白点
	float	pad;
};

// トーンマップ調整用定数バッファ
cbuffer CBToneMapOption : register(b14)
{
	ToneMapOptionData g_toneMap;
}

#endif

// ルートシグネチャ用
#define RS_TONEMAP_OPTION_CB "CBV(b14,visibility = SHADER_VISIBILITY_ALL)"
