#pragma once

//==========================================================================================
//
// 1フレーム分のヒット情報を貯めるワールドリソース。
//
// CollisionEvent が「エンティティ単位の最新ヒット(1件のみ)」なのに対して、
// こちらは「そのフレームに起きたヒット全部」をワールドに集約したもの。
//  ・弾 vs 敵      → 着弾エフェクトのプレハブ生成
//  ・敵の攻撃 vs 敵 → のけぞり、画面を赤くする、など複数の反応系が同じ1件を見る
// といった具合に、1つのヒットを複数のシステムが横から読みたいので配列で持つ。
//
// 産む: 当たり判定系(HitDetectSystem など) / 消す: HitEventClearSystem(PreUpdate)
// 読む: 反応系(エフェクト生成、のけぞり、ポストエフェクトなど)
//
//==========================================================================================

// ヒットの種別。反応系が「何に当たったか」で分岐するために使う。
enum class EHitEventType : uint8_t
{
	Unknown = 0,	// 未設定
	Bullet,			// 弾が当たった
	Melee,			// 近接攻撃が当たった
	Explosion,		// 爆風が当たった
};

// ヒット1件分の情報
struct HitEvent
{
	// 当てた側(弾やダメージ判定を出した敵など)
	Engine::ECS::Entity attacker = Engine::ECS::Limits::INVALID_ENTITY;

	// 当てた側の持ち主(弾を撃った本体)。
	// attacker は弾そのものなので、「自分の攻撃が当たったか」を知りたい側
	// (ヒットマーカーなど)は attacker ではなくこちらを見る。
	// 弾以外で持ち主が居ない場合は無効値のまま。
	Engine::ECS::Entity shooter = Engine::ECS::Limits::INVALID_ENTITY;

	// 当てられた側(敵やプレイヤー)
	Engine::ECS::Entity victim = Engine::ECS::Limits::INVALID_ENTITY;

	DirectX::XMFLOAT3 hitPos = { 0.0f, 0.0f, 0.0f };	// 当たった位置(エフェクト発生点)
	DirectX::XMFLOAT3 hitDir = { 0.0f, 0.0f, 0.0f };	// victim から見て受けた方向(のけぞりの向きに使う)

	float damage = 0.0f;							// 与えたダメージ(未使用なら0)
	EHitEventType type = EHitEventType::Unknown;	// ヒットの種別

	// 生成したいエフェクトのプレハブ(未設定なら生成しない)。
	// 弾のように「反応する前に自分が消える」側が、産むときに指定しておくために持たせている。
	Engine::GUID effectPrefabGUID = Engine::DefaultGUID;
};

// ワールドに1つだけ置くヒットイベントの配列
struct HitEventResource
{
	// そのフレームに発生したヒット群
	std::vector<HitEvent> events = {};

	// ヒットを追加する
	void Push(const HitEvent& a_event)
	{
		events.push_back(a_event);
	}

	// フレーム頭のクリア。
	// capacity は残すので、毎フレームの再確保は起きない。
	void Clear()
	{
		events.clear();
	}

	// 事前確保(初期化時に一度だけ呼ぶ想定)
	void Reserve(size_t a_capacity)
	{
		events.reserve(a_capacity);
	}

	// 指定エンティティが受けたヒットがあるか(画面を赤くする等の判定用)
	bool HasVictim(const Engine::ECS::Entity& a_entity) const
	{
		for (const HitEvent& _event : events)
		{
			if (_event.victim == a_entity) return true;
		}
		return false;
	}
};
