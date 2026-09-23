#pragma once

//==========================================================================================
//
// EffectPrefab(炊いたら時間で消える大きな演出)を出すためのヘルパー。
//
// エフェクトプレハブは必ずここを通して出す。ここで全ノードに寿命を付けるので、
// プレハブ側に LifeTimeComponent を入れ忘れていても、負(無期限)を入れていても、
// アセットの lifeTime を過ぎて残るものは無い(自滅の保証はここが担う)。
//
// ・生成は遅延コマンドに積む(実体化は次の BeginFrame)。
//   システムの反復中からでも、GameObject の Update からでも呼んでよい。
// ・ルートには LocalTransform と WorldMatrix を足す。エフェクトもモデルも
//   ワールド行列から出るので、入れ忘れると何も見えないため。
// ・破片の初速のように「出す側だけが知っている値」は a_edit で材料へ書き込む。
//
//==========================================================================================

namespace Engine
{
	namespace Resource { class EffectPrefab; struct PrefabInstanceData; class ResourceManager; }
}

namespace App::Utility
{
	// 生成直前に材料へ手を入れる。先頭がルートで、親が子より前に並ぶ
	using EffectPrefabEditFunc =
		std::function<void(Engine::ECS::World&, std::vector<Engine::Resource::PrefabInstanceData>&)>;

	/// <summary>
	/// エフェクトプレハブを指定座標に出す(遅延コマンドを積むだけ)
	/// </summary>
	/// <param name="a_effectPrefab">出すもの</param>
	/// <param name="a_pos">ルートの位置(ワールド)</param>
	/// <param name="a_edit">生成直前に材料へ手を入れる(無くてよい)</param>
	/// <returns>生成コマンドを積めたら true(中身が空なら false)</returns>
	bool SpawnEffectPrefab(
		Engine::ECS::World& a_world,
		const Engine::Resource::EffectPrefab& a_effectPrefab,
		const Math::Vector3& a_pos,
		const EffectPrefabEditFunc& a_edit = {});

	/// <summary>
	/// GUID から引いて出す。読み込みが済んでいなければ出さない(ここで読み込みは始めない)
	/// </summary>
	bool SpawnEffectPrefab(
		Engine::ECS::World& a_world,
		Engine::Resource::ResourceManager& a_resourceManager,
		Engine::Handle<Engine::Resource::EffectPrefab> a_handle,
		const Math::Vector3& a_pos,
		const EffectPrefabEditFunc& a_edit = {});
}
