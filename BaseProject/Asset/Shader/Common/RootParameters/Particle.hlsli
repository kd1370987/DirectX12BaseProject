#ifndef PARTICLE_CORE_HLSLI
#define PARTICLE_CORE_HLSLI

// -------------------------------------------------
// 構造体の定義
// -------------------------------------------------
struct ParticleData
{
	float3 pos; // 現在座標
	float life; // 残り寿命
	float3 velocity; // 現在の移動ベクトル
	float size; // スケール値
};

struct EmitData
{
	float3	pos;			// 発生源のワールド座標
	uint	emitCount;		// 発生させる数
	float3	emitDirection;	// 発生させたい方向
	float	baseScale;		// スケール値

	// ---- ランダム要素 ----
	float positionRadius;	// 発生位置の半径
	float directionAngle;	// 方向のばらつき角度 (度)

		// 拡縮区間
	float minScale;
	float maxScale;

		// スピード区間
	float minSpeed;
	float maxSpeed;

		// 生存時間区間
	float minLifeTime;
	float maxLifeTime;
};
#endif // PARTICLE_CORE_HLSLI

