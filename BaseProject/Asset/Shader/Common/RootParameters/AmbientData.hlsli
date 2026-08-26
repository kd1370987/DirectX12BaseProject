// シーン全体に掛かる環境光・フォグ。
// 平行光はここではなく LightManager が持つ(LightData.hlsli)。
// ※ CPU 側 Engine::Graphics::AmbientData と並びを合わせること。
//    float3 は16バイト境界をまたぐと次の境界へ押し出されるので、pad を削らないこと
#ifndef ROOTPARAM_AMBIENT_DATA_HLSLI
#define ROOTPARAM_AMBIENT_DATA_HLSLI

struct AmbientData
{
	float3 ambientColor;
	float pad;

	// 高さフォグ
	float3 heightFogColor;
	float heightFogMaxRange;	// 100%になるまでの距離(基準高さから)
	float heightFogHeight;		// 出始める高さ(ワールドY)
	int heightFogEnable;		// 0 で計算ごとスキップ
	int heightFogDenseDown;		// 1 = 下ほど濃い / 0 = 上ほど濃い
	float pad3;

	// 距離フォグ
	float3 distanceFogColor;
	float distanceFogMaxRange;	// 100%になる距離
	float distanceFogStart;		// 出始める距離
	int distanceFogEnable;		// 0 で計算ごとスキップ
	float2 pad4;
};

#endif
