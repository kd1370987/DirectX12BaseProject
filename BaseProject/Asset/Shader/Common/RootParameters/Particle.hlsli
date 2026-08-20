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

	// 発生した時点の寿命。
	// 寿命は粒ごとにランダムなので、「今どこまで進んだか(0〜1)」を出すには
	// 割る相手を粒自身が覚えているしかない。サイズ・色・フェードがこれを使う
	float startLife;

	// どの発生源の座標系で回っているか(ParticleDrawData.emitterMatrices の添字)。
	// 0 は単位行列で予約してあるので、ワールド空間で回す粒はここが 0 のまま
	uint emitterIndex;

	float2 pad;
};

// 1つのパーティクルアセットが同時に持てる発生源の数
// ※ C++ 側 Engine::Particle::PARTICLE_EMITTER_MAX と合わせること
#define PARTICLE_EMITTER_MAX 8

// 板ポリの向きの決め方
// ※ C++ 側 Engine::Particle::EParticleOrientation と数値を合わせること
#define PARTICLE_ORIENT_BILLBOARD			0	// 常にカメラ正面(従来)
#define PARTICLE_ORIENT_VELOCITY_BILLBOARD	1	// カメラ正面のまま、画面上で進行方向へ回す
#define PARTICLE_ORIENT_VELOCITY_AXIS		2	// 進行方向をワールドの縦軸にする(手前へ向かうと縮む)

// 発生方向の決め方
// ※ C++ 側 Engine::Particle::EParticleEmitShape と数値を合わせること
#define PARTICLE_EMIT_SHAPE_CONE		0	// 指定方向を軸にした円錐(噴射・従来)
#define PARTICLE_EMIT_SHAPE_SPHERE		1	// 中心から全方向へ均等(爆発)
#define PARTICLE_EMIT_SHAPE_HEMISPHERE	2	// 指定方向側の半球だけ(地面での爆発)

// 描画設定 : アセット単位で変わる値
// ※ C++ 側 Engine::Particle::ParticleDrawData と並びを合わせること
// ※ 定数バッファなので float4 単位で行が変わる。並べ替えるときは両側を必ず揃えること
struct ParticleDrawData
{
	uint	orientation;	// 上の PARTICLE_ORIENT_*
	float	stretch;		// 進行方向への伸ばし倍率
	float	endSizeScale;	// 寿命の終わりでのサイズ倍率(1で変化なし)
	float	fadeInRatio;	// 寿命のうち、頭から透明→不透明にする割合(0でフェードインなし)

	float	fadeOutRatio;	// 寿命のうち、末尾で不透明→透明にする割合(0でフェードアウトなし)
	float3	pad;

	float4	startColor;		// 発生時の色(RGBは1を超えてよい。ブルームが乗る)
	float4	endColor;		// 消える直前の色

	//----------------------------------------------------------------------
	// 発生源の行列
	//
	// ローカル空間で回している粒を、描くときにワールドへ戻すためのもの。
	// 添字 0 は単位行列で予約してあるので、ワールド空間で回している粒は
	// そのまま掛けても何も起きない(分岐が要らない)。
	//----------------------------------------------------------------------
	row_major float4x4 emitterMatrices[PARTICLE_EMITTER_MAX];
};

struct EmitData
{
	float3	pos;			// 発生源のワールド座標
	uint	emitCount;		// 発生させる数
	float3	emitDirection;	// 発生させたい方向
	float	baseScale;		// スケール値

	// ---- ランダム要素 ----
	float positionRadius;	// 発生位置の半径
	float directionAngle;	// 方向のばらつき角度 (ラジアン。Cone のときだけ使う)

		// 拡縮区間
	float minScale;
	float maxScale;

		// スピード区間
	float minSpeed;
	float maxSpeed;

		// 生存時間区間
	float minLifeTime;
	float maxLifeTime;

	// 発生方向の決め方 : 上の PARTICLE_EMIT_SHAPE_*
	uint	emitShape;

	// 出した粒に持たせる発生源の番号(ワールド空間なら 0)
	uint	emitterIndex;

	float2	shapePad;
};
#endif // PARTICLE_CORE_HLSLI
