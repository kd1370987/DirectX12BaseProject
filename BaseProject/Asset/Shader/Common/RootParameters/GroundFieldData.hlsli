// グラウンドフィールドの衝撃と定数。GroundFieldPass が送る。
//
//   衝撃はアプリ側が SceneView::AddGroundImpulse で毎フレーム積んだもの。
//   StructuredBuffer は要素数を持たないので、数は GroundFieldData で渡す。
//
// ※ CPU 側 Engine::Graphics::GroundImpulse / GroundFieldCB と並びを合わせること
#ifndef ROOTPARAM_GROUND_FIELD_DATA_HLSLI
#define ROOTPARAM_GROUND_FIELD_DATA_HLSLI

// グラウンドフィールドに伝える衝撃
struct GroundImpulse
{
	float3 pos;
	float radius;

	float strength;
	float speed;
	float width;
	float lifetime;

	float age;			// 衝撃を出してからの経過時間(秒)。積む側が毎フレーム進める
	float3 pad0;
};

// グラウンドフィールドの定数
struct GroundFieldData
{
	float time;			// パスが回り始めてからの経過時間(秒)
	float deltaTime;	// 前フレームからの経過時間(秒)
	uint impulseCount;	// 今フレームの衝撃の数
	float pad0;
};

#endif
