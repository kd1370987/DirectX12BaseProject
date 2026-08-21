// インクルードガード
#ifndef CB_SKY_OPTION_HLSLI
#define CB_SKY_OPTION_HLSLI

// オプション(SkyOption)からCPU側で詰めて送られてくる調整値。
// スカイパスはディファードライティングの後ろで HDR バッファへ直接描くので、
// ここで掛けた明るさがそのままブルームの抽出しきい値とトーンマップに乗る。
// ※ C++ 側 SkyOptionData と並びを合わせること
struct SkyOptionData
{
	float exposure;		// スカイの色に掛ける露出倍率
	float3 pad;
};

// スカイ調整用定数バッファ
cbuffer CBSkyOption : register(b15)
{
	SkyOptionData g_sky;
}

#endif

// ルートシグネチャ用
#define RS_SKY_OPTION_CB "CBV(b15,visibility = SHADER_VISIBILITY_ALL)"
