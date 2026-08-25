// 川瀬式ブルームの調整値。抽出パスと合成パスの両方が同じものを読む。
//
//   抽出 : threshold を超えた輝度ぶんを取り出す
//   合成 : メインカラー + ブルーム * intensity
//
// ※ CPU 側 Engine::Graphics::BloomOptionCB と並びを合わせること
#ifndef ROOTPARAM_BLOOM_OPTION_DATA_HLSLI
#define ROOTPARAM_BLOOM_OPTION_DATA_HLSLI

struct BloomOptionData
{
	float threshold;
	// しきい値付近を二次曲線でつなぐ幅の割合。0 でハードカット。
	// ハードカットのままだと輝度がしきい値をまたぐ画素がフレーム間でちらつく
	float softKnee;
	float intensity;
	int   enable;
};

#endif
