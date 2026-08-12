#pragma once

//==========================================================================================
//
// 1フレーム分の「死亡」を貯めるワールドリソース。
//
// 死因はいろいろある(体力切れ・着弾・寿命など)が、
// 死んだときの後始末(エフェクトを出す等)は死因ごとに書きたくない。
// そこで「誰がどこで死んだか」だけをここへ積み、反応する側が横から読む形にしている。
// HitEventResource と同じ考え方。
//
// 積む : 死亡を決めるシステム(HealthSystem / ExplodeOnHitSystem など)
// 読む : DeathEffectSystem(読み終わったら自分でクリアする)
//
// 死亡したエンティティは AddReleaseEntity で解放予約された状態で、
// 実際に消えるのは次フレームの BeginFrame。読む側が動くのは同じフレームの後半なので、
// entity からコンポーネントを引くことはまだできる。
// ただし消えた後の位置は引けないので、座標は積む側が入れておくこと。
//
//==========================================================================================

// 死亡1件分の情報
struct DeathEvent
{
	// 死んだエンティティ
	Engine::ECS::Entity entity = Engine::ECS::Limits::INVALID_ENTITY;

	// 死んだ位置(ワールド)。エフェクトの発生点に使う
	DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
};

// ワールドに1つだけ置く死亡イベントの配列
struct DeathEventResource
{
	std::vector<DeathEvent> events = {};

	// 死亡を追加する
	void Push(const DeathEvent& a_event)
	{
		events.push_back(a_event);
	}

	// capacity は残すので、毎フレームの再確保は起きない
	void Clear()
	{
		events.clear();
	}

	// 事前確保(初期化時に一度だけ呼ぶ想定)
	void Reserve(size_t a_capacity)
	{
		events.reserve(a_capacity);
	}
};
