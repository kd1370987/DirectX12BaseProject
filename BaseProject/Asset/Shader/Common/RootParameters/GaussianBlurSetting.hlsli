// 汎用ガウシアンブラーの設定。入力と出力の解像度が違ってよいので縮小・拡大も兼ねる。
// タップ数は (tapRadius*2+1)^2 なので、フル解像度側では小さく渡すこと
#ifndef ROOTPARAM_GAUSSIAN_BLUR_SETTING_HLSLI
#define ROOTPARAM_GAUSSIAN_BLUR_SETTING_HLSLI

struct GaussianBlurSetting
{
	// 入力1テクセルぶんのUV(= 1 / 入力解像度)。
	// 出力側の刻みにすると、拡大時にオフセットが入力の1テクセル未満になりブラーが効かない
	float2 srcTexelSize;
	float  sigma;			// 標準偏差(入力テクセル単位)
	int    tapRadius;		// 片側のタップ数。0 でブラーなし
};

#endif
