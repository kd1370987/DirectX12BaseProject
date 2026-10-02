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

// 衝撃1つが、震源から a_distance 離れた位置へ与える波の強さ。
// 地面(GroundFieldCS)とフォグ(GroundVolumetricFogCS)で同じ式を使うためにここへ置く
float CalcGroundImpulseWave(GroundImpulse a_impulse, float a_distance)
{
	// 衝撃を出してからの経過時間
	float _age = a_impulse.age;
	if (_age < 0.0f) return 0.0f;

	float _waveRadius = _age * a_impulse.speed;						// 衝撃波の出現位置
	float _waveWidth = max(a_impulse.radius, 0.0001f);				// 衝撃波の厚み
	float _waveDistance = abs(a_distance - _waveRadius);			// 波の中心からの距離
	float _wave = 1.0f - saturate(_waveDistance / _waveWidth);		// 波の付近だけ影響させる
	float _fade = exp(-_age * 2.0f);								// 経過時間で減衰

	return _wave * a_impulse.strength * _fade;
}

// グラウンドフィールドの定数
struct GroundFieldData
{
	float time;			// パスが回り始めてからの経過時間(秒)
	float deltaTime;	// 前フレームからの経過時間(秒)
	uint impulseCount;	// 今フレームの衝撃の数
	float pad0;
};

#endif
