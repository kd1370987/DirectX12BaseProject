// 魚眼レンズの調整値。アクティブカメラの FishEyeComponent を FishEyePass へ送る。
//
//   center から外へ向かうほど強く引き伸ばして、レンズ越しに覗いたような歪みを出す。
//
//   ずらし量は中心からの距離の二乗に比例する(strength * r * r)ので、
//   中心付近はほとんど動かず、画面の隅だけが大きく膨らむ。
//
//   strength が正なら樽型(外側が縮んで四隅に黒が出る)、
//   負なら糸巻き型(外側が伸びて画面外まで拡がる)。
//
// ※ CPU 側 Engine::Graphics::FishEyeOptionCB と並びを合わせること
#ifndef ROOTPARAM_FISH_EYE_OPTION_DATA_HLSLI
#define ROOTPARAM_FISH_EYE_OPTION_DATA_HLSLI

struct FishEyeOptionData
{
	float2 center;		// 歪みの中心(UV : 画面左上が0、右下が1)
	float strength;		// 歪みの強さ(0で歪まない)
	int enable;			// 0 でそのまま通す
};

#endif
