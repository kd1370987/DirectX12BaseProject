// 主光源のシャドウマップ(カスケード)。
// ※ CPU 側 Engine::Graphics::SunShadowCB と並びを合わせること。
//    cbuffer なので16バイト行に揃える
#ifndef ROOTPARAM_SHADOW_MAP_DATA_HLSLI
#define ROOTPARAM_SHADOW_MAP_DATA_HLSLI

// カスケードの最大数 : シャドウマップは 2x2 のタイルに分けた1枚のアトラス
#define SHADOW_MAX_CASCADES 4

struct ShadowMapData
{
	// カスケードごとのライトのビュー射影
	float4x4 lightViewProj[SHADOW_MAX_CASCADES];

	// カスケードごとの値。要素 i がカスケード i
	float4 cascadeFar;			// このカスケードが受け持つ奥行きの終わり(ビュー空間Z)
	float4 cascadeBlendStart;	// ここから次のカスケードへ混ぜ始める(ビュー空間Z)
	float4 cascadeRadius;		// 箱の半径(ワールド)
	float4 cascadeDepthRange;	// 箱の奥行き(ワールド)

	float3 lightDir;			// 光の進む向き
	uint cascadeCount;			// 0 なら影なし(全部ひなた)

	float depthBias;			// 深度の比較にかける余裕(ワールドの長さ)
	float normalBias;			// 法線方向へずらす量(テクセル数)
	float softness;				// 縁のぼかし幅(テクセル数)
	float distance;				// 影の届く範囲(ビュー空間の奥行き)。影の求め方によらず入る
};

#endif
