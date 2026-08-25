// ライティングの調整値。OptionManager の LightingOption を毎フレーム流し込む
#ifndef ROOTPARAM_LIGHTING_OPTION_DATA_HLSLI
#define ROOTPARAM_LIGHTING_OPTION_DATA_HLSLI

struct LightingOptionData
{
	float giIntensity;
	float directionalIntensity;
	float dielectricF0;			// 非金属の基本反射率(スペキュラF0)
	float pad;
};

#endif
