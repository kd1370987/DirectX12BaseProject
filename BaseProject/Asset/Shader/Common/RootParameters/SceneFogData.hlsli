// シーン全体に掛かるボリュメトリックフォグの調整値。SceneVolumetricFogPass が送る。
//
// ※ CPU 側 Engine::Graphics::SceneFogCB と並びを合わせること
#ifndef ROOTPARAM_SCENE_FOG_DATA_HLSLI
#define ROOTPARAM_SCENE_FOG_DATA_HLSLI

struct SceneFogData
{
	float3 fogColor;	// フォグの色
	float density;		// 濃さ(1m あたり)。0 ならシーンのフォグは掛からない

	float maxDistance;	// 空(何も描かれていない画素)へ向けて積分する距離(m)
	float3 pad0;
};

#endif
