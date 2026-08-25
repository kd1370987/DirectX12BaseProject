// 低解像度バッファをGBufferに合わせて引き伸ばすときの許容差
#ifndef ROOTPARAM_UPSCALE_SETTING_HLSLI
#define ROOTPARAM_UPSCALE_SETTING_HLSLI

struct UpScaleSetting
{
	float scaleRatio;
	float depthSigma;		// 深度の許容差(ビュー深度に対する相対値)。小さいほど厳密にエッジで分ける
	float normalPower;		// 法線の許容差(pow の指数)。大きいほど少しの角度差で分ける
	float pad;
};

#endif
