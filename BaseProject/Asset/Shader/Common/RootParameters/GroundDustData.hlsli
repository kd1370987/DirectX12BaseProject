// 地面から一定の高さまで漂うチリ(グラウンドダスト)の調整値。SceneVolumetricFogPass が送る。
//
// 衝撃でチリが払われる・寄せられる量は GroundFieldPass がテクスチャへ書いたものを引く。
//
// ※ CPU 側 Engine::Graphics::GroundDustCB と並びを合わせること
#ifndef ROOTPARAM_GROUND_DUST_DATA_HLSLI
#define ROOTPARAM_GROUND_DUST_DATA_HLSLI

struct GroundDustData
{
	float3 dustColor;	// チリの色
	float density;		// 濃さ(1m あたり)。0 ならチリは出ない

	float height;		// チリが立つ高さ(地面から。この高さで濃さが 0 になる)
	float noiseScale;	// ノイズのワールド座標に掛ける倍率(大きいほど細かい)
	float time;			// パスが回り始めてからの経過時間(秒)。ノイズを流すのに使う
	float stepSize;		// チリの層の中をレイマーチする1歩の長さ(m)
};

#endif
