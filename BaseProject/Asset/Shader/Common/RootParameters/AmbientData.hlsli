// インクルードガード
#ifndef CB_AMBIENT_HLSLI
#define CB_AMBIENT_HLSLI

// ※ CPU 側(Source/Engine/Graphics/CBData.h の AmbientData)と並びを合わせること
struct AmbientData
{
	// 環境光
	float3 ambientColor;
	float pad;
	// ディレクショナルライト
	float3 DL_Dir;			// ライトの方向（ワールド空間）
	float pad1;
	float3 DL_Color;		// ライトの色
	float pad2;

	// 高さフォグ
	float3 heightFogColor;		// フォグの色
	float heightFogMaxRange;	// 100% になるまでの距離（基準高さから）
	float heightFogHeight;		// フォグが出始める高さ（ワールドY）
	int heightFogEnable;		// 0 なら計算ごとスキップ
	int heightFogDenseDown;		// 1 = 下へ行くほど濃い / 0 = 上へ行くほど濃い
	float pad3;

	// 距離フォグ
	float3 distanceFogColor;	// フォグの色
	float distanceFogMaxRange;	// 100% になる距離
	float distanceFogStart;		// フォグが出始める距離
	int distanceFogEnable;		// 0 なら計算ごとスキップ
	float2 pad4;
};

// カメラの定数バッファ
cbuffer CBAmbient : register(b10)
{
	AmbientData g_ambient;
}

#endif

// 共通定数バッファ
#define RS_AMBIENT_CB "CBV(b10,visibility = SHADER_VISIBILITY_ALL)"
