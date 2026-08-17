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

// 板ポリの向きの決め方
// ※ C++ 側 Engine::Particle::EParticleOrientation と数値を合わせること
#define PARTICLE_ORIENT_BILLBOARD			0	// 常にカメラ正面(従来)
#define PARTICLE_ORIENT_VELOCITY_BILLBOARD	1	// カメラ正面のまま、画面上で進行方向へ回す
#define PARTICLE_ORIENT_VELOCITY_AXIS		2	// 進行方向をワールドの縦軸にする(手前へ向かうと縮む)

// 描画設定 : アセット単位で変わる値
// ※ C++ 側 Engine::Particle::ParticleDrawData と並びを合わせること
struct ParticleDrawData
{
	uint	orientation;	// 上の PARTICLE_ORIENT_*
	float	stretch;		// 進行方向への伸ばし倍率
	float2	pad;
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

