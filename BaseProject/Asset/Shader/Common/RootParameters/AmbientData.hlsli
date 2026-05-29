// インクルードガード
#ifndef CB_AMBIENT_HLSLI
#define CB_AMBIENT_HLSLI

struct AmbientData
{
	// 環境光
	float3 ambientColor;
	
	// ディレクショナルライト
	float3 DL_Dir;			// ライトの方向（ワールド空間）
	float3 DL_Color;		// ライトの色
};

// カメラの定数バッファ
cbuffer CBAmbient : register(b10)
{
	AmbientData g_ambient;
}

#endif

// 共通定数バッファ
#define RS_AMBIENT_CB "CBV(b10,visibility = SHADER_VISIBILITY_ALL)"
