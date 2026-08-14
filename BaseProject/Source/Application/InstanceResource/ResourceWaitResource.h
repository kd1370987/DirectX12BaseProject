#pragma once

//==========================================================================================
// リソースの到着待ちエンティティ
//
// 非同期ロードを投げたエンティティは、実体が届くまで Start フェーズへ進めない。
// Start 系システム(AnimationModelStartSystem など)は
// ノード数やボーン数をもとに領域を確保するため、
// 空のリソースで走らせると出来上がりが壊れたまま二度と直らない。
//
// ---- なぜ Start の中でスキップしないのか ----
// 同じエンティティに Start 系が複数ぶら下がっている。
// Start の中で個別にスキップすると「一部だけ走った」状態になり、
// 翌フレームに再実行したときに走り済みのものが二重に走る。
// AnimationModelStartSystem は RangePool の確保を行うため、
// 二重実行はそのまま確保のリークになる。
//
// そこで AwakeTag -> StartTag の遷移そのものを止める。
// StartTag が立つのは「必要なものが全部揃ったとき」だけになるので、
// Start 系のコードは一切変えなくてよい。
//
// ---- 使い方 ----
// Awake フェーズのゲートシステムが、まだ届いていないエンティティをここへ登録する。
// World::BeginFrame() が遷移時にこれを見て、登録されているものを残す。
// 中身は毎フレーム作り直すので、フレームをまたいで持ち越さない
//==========================================================================================
struct ResourceWaitResource
{
	// このフレームは Start へ進めないエンティティ
	std::unordered_set<Engine::ECS::Entity> waitingEntities = {};

	// 待たせた回数 : 解決漏れに気づくためのカウンタ
	std::unordered_map<Engine::ECS::Entity, uint32_t> waitFrameCounts = {};

	/// <summary>
	/// このエンティティを待たせる
	/// </summary>
	void AddWait(Engine::ECS::Entity a_entity)
	{
		waitingEntities.insert(a_entity);
	}

	bool IsWaiting(Engine::ECS::Entity a_entity) const
	{
		return waitingEntities.contains(a_entity);
	}
};
