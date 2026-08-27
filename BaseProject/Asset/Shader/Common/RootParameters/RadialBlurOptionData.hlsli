// ラジアルブラーの調整値。アクティブカメラの RadialBlurComponent を RadialBlurPass へ送る。
//
//   blurCenter から放射状に伸びる方向へサンプルを引きずって、
//   スピード感/衝撃を出す画面効果。
//
//   中心から radius(UV) の内側はボカさない。そこから falloff の傾きで効きが立ち上がる。
//
// ※ CPU 側 Engine::Graphics::RadialBlurOptionCB と並びを合わせること
#ifndef ROOTPARAM_RADIAL_BLUR_OPTION_DATA_HLSLI
#define ROOTPARAM_RADIAL_BLUR_OPTION_DATA_HLSLI

struct RadialBlurOptionData
{
	float2 blurCenter;	// ブラーの中心(UV : 画面左上が0、右下が1)
	float strength;		// 引きずる長さ(UV単位。中心からの距離に比例して伸びる)
	int sampleCount;	// サンプル数(多いほど滑らかだが重い)

	float radius;		// ここまで(中心からのUV距離)はボカさない
	float falloff;		// radius から先の効きの立ち上がり(大きいほど急に効く)
	int enable;			// 0 でそのまま通す
	float pad0;
};

#endif
