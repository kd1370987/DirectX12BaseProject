// グラウンドフィールドの衝撃と定数。GroundFieldPass が送り、フィールドのテクスチャを書く。
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
// GroundFieldCS がフィールドのテクスチャを書くときに使う
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

// 衝撃1つが、震源から a_distance 離れた位置をどれだけ払ったか(0..1)。
// 波が通り過ぎた内側ほど 1 に近く、波頭より外は 0。
// 経過時間で減衰するので、時間が経つと払った場所が元へ戻る
float CalcGroundImpulseSweep(GroundImpulse a_impulse, float a_distance)
{
	float _age = a_impulse.age;
	if (_age < 0.0f) return 0.0f;

	float _waveRadius = _age * a_impulse.speed;						// 衝撃波の出現位置
	float _waveWidth = max(a_impulse.radius, 0.0001f);				// 衝撃波の厚み
	float _inside = saturate((_waveRadius - a_distance) / _waveWidth);	// 波頭から内側へ厚みぶんで 0→1
	float _fade = exp(-_age * 2.0f);								// 経過時間で減衰

	return saturate(_inside * a_impulse.strength * _fade);
}

//------------------------------------------------------------------------------------------
// グラウンドフィールドのテクスチャ
//
// カメラを中心にした GROUND_FIELD_WORLD_SIZE (m) 四方を、真上から xz で並べたもの。
//   r = 払われずに残ったチリの量(1 = 手つかず) / g = 波頭に寄せられたチリの量
// 範囲の外は「衝撃なし」(r = 1, g = 0)として扱う。
//
// 中心はカメラの位置をテクセル単位に丸めたもの。丸めないとカメラが動くたびに
// テクセルの境目がずれて、波の縁がちらつく。
// 書く側(GroundFieldCS)と読む側(SceneVolumetricFogCS)が同じ式で求めるので、
// 中心をどこかへ渡す必要はない(どちらも同じフレームのカメラを見ている)
//
// ※ CPU 側 Engine::Graphics::GROUND_FIELD_WORLD_SIZE と合わせること
//------------------------------------------------------------------------------------------
#define GROUND_FIELD_WORLD_SIZE 256.0f

// フィールドの中心(xz)。a_resolution はテクスチャの1辺のテクセル数
float2 CalcGroundFieldCenter(float3 a_cameraPos, float a_resolution)
{
	const float _texelSize = GROUND_FIELD_WORLD_SIZE / a_resolution;
	return floor(a_cameraPos.xz / _texelSize) * _texelSize;
}

// 水平位置(xz) -> フィールドのUV。範囲の外は 0..1 を外れる
float2 GroundFieldWorldToUV(float2 a_posXZ, float2 a_center)
{
	return (a_posXZ - a_center) / GROUND_FIELD_WORLD_SIZE + 0.5f;
}

// フィールドのUV -> 水平位置(xz)
float2 GroundFieldUVToWorld(float2 a_uv, float2 a_center)
{
	return a_center + (a_uv - 0.5f) * GROUND_FIELD_WORLD_SIZE;
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
