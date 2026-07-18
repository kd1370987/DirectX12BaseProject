// インクルードガード
#ifndef CB_LIGHTING_OPTION_HLSLI
#define CB_LIGHTING_OPTION_HLSLI

// オプション(LightingOption)からCPU側で詰めて送られてくる調整値
struct LightingOptionData
{
	float giIntensity;			// GI(間接光)の強さ
	float directionalIntensity;	// 平行光(直接光)の強さ
	float dielectricF0;			// 非金属の基本反射率(スペキュラF0)
	float pad;
};

// ライティング調整用定数バッファ
cbuffer CBLightingOption : register(b11)
{
	LightingOptionData g_lightingOp;
}

#endif

// ルートシグネチャ用
#define RS_LIGHTING_OPTION_CB "CBV(b11,visibility = SHADER_VISIBILITY_ALL)"
