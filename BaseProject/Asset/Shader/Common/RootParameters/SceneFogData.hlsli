// シーン全体に掛かるボリュメトリックフォグの調整値。シーンの環境設定(SceneAmbient)の値を SceneVolumetricFogPass が送る。
//
// ※ CPU 側 Engine::Graphics::SceneFogCB と並びを合わせること
#ifndef ROOTPARAM_SCENE_FOG_DATA_HLSLI
#define ROOTPARAM_SCENE_FOG_DATA_HLSLI

struct SceneFogData
{
	float3 fogColor;	// フォグの色(届いた光に掛ける)
	float density;		// 濃さ(1m あたり)。0 ならシーンのフォグは掛からない

	float maxDistance;	// 空(何も描かれていない画素)へ向けて積分する距離(m)
	float anisotropy;	// 平行光を散らす向きの偏り(-1..1)。正で光源の方向を見たときに明るい
	float lightScale;	// 媒質に届く光(環境光 + 平行光)に掛ける倍率
	float pad0;
};

#endif
