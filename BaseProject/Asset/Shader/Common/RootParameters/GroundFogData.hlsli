// 地面付近のボリュメトリックフォグの調整値。GroundVolumetricFogPass が送る。
//
// ※ CPU 側 Engine::Graphics::GroundFogCB と並びを合わせること
#ifndef ROOTPARAM_GROUND_FOG_DATA_HLSLI
#define ROOTPARAM_GROUND_FOG_DATA_HLSLI

struct GroundFogData
{
	float fogHeight;	// フォグが立つ高さ(この高さで濃さが 0 になる)
	float density;		// 濃さ
	float noiseScale;	// ノイズのワールド座標に掛ける倍率(大きいほど細かい)
	float time;			// パスが回り始めてからの経過時間(秒)。ノイズを流すのに使う

	float3 fogColor;	// フォグの色
	float stepSize;		// レイマーチの1歩の長さ(m)

	uint impulseCount;	// 今フレームの衝撃の数(StructuredBuffer は要素数を持たないので)
	float3 pad0;
};

#endif
