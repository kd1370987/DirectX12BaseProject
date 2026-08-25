// GPUパーティクルのバッファ要素とディスパッチ設定。
// ※ CPU 側 Engine::Particle 以下の同名構造体と並びを合わせること
#ifndef ROOTPARAM_PARTICLE_HLSLI
#define ROOTPARAM_PARTICLE_HLSLI

// 1つのパーティクルアセットが同時に持てる発生源の数
// ※ CPU 側 Engine::Particle::PARTICLE_EMITTER_MAX と合わせること
#define PARTICLE_EMITTER_MAX 8

// 板ポリの向きの決め方
// ※ CPU 側 Engine::Particle::EParticleOrientation と数値を合わせること
#define PARTICLE_ORIENT_BILLBOARD			0	// 常にカメラ正面
#define PARTICLE_ORIENT_VELOCITY_BILLBOARD	1	// カメラ正面のまま、画面上で進行方向へ回す
#define PARTICLE_ORIENT_VELOCITY_AXIS		2	// 進行方向をワールドの縦軸にする(手前へ向かうと縮む)

// 発生方向の決め方
// ※ CPU 側 Engine::Particle::EParticleEmitShape と数値を合わせること
#define PARTICLE_EMIT_SHAPE_CONE		0	// 指定方向を軸にした円錐(噴射)
#define PARTICLE_EMIT_SHAPE_SPHERE		1	// 中心から全方向へ均等(爆発)
#define PARTICLE_EMIT_SHAPE_HEMISPHERE	2	// 指定方向側の半球だけ(地面での爆発)

// 粒1つぶんの状態。更新も描画も同じバッファを読む
struct ParticleData
{
	float3 pos;
	float life;				// 残り寿命
	float3 velocity;
	float size;

	// 発生時点の寿命。寿命は粒ごとにランダムなので、
	// 進行度(0〜1)を出すには割る相手を粒自身が覚えているしかない
	float startLife;

	// ParticleDrawData.emitterMatrices の添字。
	// 0 は単位行列で予約してあるので、ワールド空間で回す粒は 0 のまま
	uint emitterIndex;

	float2 pad;
};

// アセット単位の描画設定。
// 定数バッファなので float4 単位で行が変わる。並べ替えるときはCPU側も必ず揃えること
struct ParticleDrawData
{
	uint	orientation;	// 上の PARTICLE_ORIENT_*
	float	stretch;		// 進行方向への伸ばし倍率
	float	endSizeScale;	// 寿命の終わりでのサイズ倍率(1で変化なし)
	float	fadeInRatio;	// 寿命のうち頭で透明→不透明にする割合(0でなし)

	float	fadeOutRatio;	// 寿命のうち末尾で不透明→透明にする割合(0でなし)
	float3	pad;

	float4	startColor;		// RGBは1を超えてよい(超えたぶんにブルームが乗る)
	float4	endColor;

	// ローカル空間で回している粒を描画時にワールドへ戻す。
	// 添字0を単位行列で予約してあるので、ワールド空間の粒も分岐なしで同じ経路を通る
	row_major float4x4 emitterMatrices[PARTICLE_EMITTER_MAX];
};

// 発生命令1件ぶん
struct EmitData
{
	float3	pos;			// 発生源のワールド座標
	uint	emitCount;
	float3	emitDirection;
	float	baseScale;

	// ---- ばらつき ----
	float positionRadius;	// 発生位置の半径
	float directionAngle;	// 方向のばらつき角度(ラジアン。Cone のみ)

	float minScale;
	float maxScale;

	float minSpeed;
	float maxSpeed;

	float minLifeTime;
	float maxLifeTime;

	uint	emitShape;		// 上の PARTICLE_EMIT_SHAPE_*
	uint	emitterIndex;	// 出した粒に持たせる発生源の番号(ワールド空間なら 0)

	float2	shapePad;
};

// 発生ディスパッチ1回ぶんの設定
struct ParticleEmitSetting
{
	uint requestCount;		// 今回処理する発生命令の数
	uint frameSeed;			// フレームごとに変わる乱数の種
};

// 更新ディスパッチ1回ぶんの設定
struct ParticleUpdateSetting
{
	float deltaTime;
	float3 gravity;

	float drag;				// 速度の減衰(1秒あたりの割合)。0 で減衰しない
	float3 pad;
};

#endif
