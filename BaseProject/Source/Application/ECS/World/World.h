#pragma once
//==========================================================================================
//
// App::ECS::World
//
// ゲーム用のワールド。Engine::ECS::World(ECSの基盤)に、ゲーム側の決めごとを載せる。
//
// ---- なぜ分けたか ----
// 以前は Engine::ECS::World が Application のコンポーネントを直接 include していた。
//   ・World.h   -> ActiveTag / AwakeTag / StartTag / PostDeserializeTag / ReleaseTag
//   ・World.cpp -> GUIDComponent / HierarchyComponent / HierarchyResource / ResourceWaitResource
// つまり ECS の中核がゲーム固有の型なしには成立せず、汎用のECS基盤ではなかった。
//
// 基盤が持つのは「エンティティの入れ物」と「システムを回す仕組み」だけ。
// 次のようなゲーム側の決めごとは、すべてこのクラスが持つ。
//
//   ・ライフサイクル : PostDeserialize -> Awake -> Start -> Active / Release の進行
//   ・親子関係       : 親を消したら子も一緒に後始末を通してから消す
//   ・GUID           : 保存をまたいで残る識別子からエンティティを引く
//   ・リソース待ち   : 実体が届くまで Start へ進めない
//
// 基盤へは仮想関数とフックで差し込んでいるので、基盤側は App を一切知らない。
//
//==========================================================================================

#include "../../../Engine/ECS/World/World.h"

#include "../PhaseTag/PhaseTag.h"
#include "../ISystem/ISystem.h"

namespace App::ECS
{
	// 基盤の型をそのまま使う
	using Entity			= Engine::ECS::Entity;
	using Signature			= Engine::ECS::Signature;
	using ComponentTypeID	= Engine::ECS::ComponentTypeID;
	using ESystemType		= Engine::ECS::ESystemType;
	using ArchetypeChunk	= Engine::ECS::ArchetypeChunk;
	using SystemContext		= Engine::ECS::SystemContext;
	using ChangeEntityCmd	= Engine::ECS::ChangeEntityCmd;

	template<typename... T> using Exclude	= Engine::ECS::Exclude<T...>;
	template<typename... T> using ReadList	= Engine::ECS::ReadList<T...>;
	template<typename... T> using WriteList	= Engine::ECS::WriteList<T...>;

	class World : public Engine::ECS::World
	{
	public:

		using Base = Engine::ECS::World;

		// 自分が使うシングルトンリソースはここで確保する
		// (BeginFrame が毎フレーム引くので、無いと成立しない)
		World();

		//==================================================================================
		//
		// 基盤から差し替えるもの
		//
		//==================================================================================

		/// <summary>フレームの先頭処理 : 初期化フェーズを1フレームで通しきる</summary>
		void BeginFrame() override;

		/// <summary>解放 : 後始末のフェーズを通してから全部消す</summary>
		void Release() override;

		/// <summary>
		/// エンティティに ReleaseTag を付けて解放予約する : 削除はすべてこれを通す
		/// </summary>
		/// <remarks>
		/// 次の BeginFrame で ActiveTag が外れて Release フェーズが走り、そのまま削除される。
		/// 借りているものを返してから消えるので、寿命切れの弾やエフェクト、
		/// 撃破された敵、エディターでの削除で各種プールが漏れない。
		/// </remarks>
		void AddReleaseEntity(const Entity& a_entity) override;

		/// <summary>GUIDからエンティティを探す</summary>
		Entity GetEntity(const Engine::GUID& a_guid) override;

		/// <summary>ゲーム固有のコンポーネント / システムを登録する</summary>
		void RegisterGameTypes() override;

		//==================================================================================
		//
		// システムの登録
		//
		//==================================================================================

		/// <summary>システムを生成して Init(タスク登録)まで済ませ、寿命を基盤へ預ける</summary>
		template<typename System>
		void RegisterSystem();

		//==================================================================================
		//
		// フェーズごとのタスク登録
		//
		// 先頭にフェーズタグを足して基盤の RegisterTask を呼ぶだけ。
		// タグは絞り込みにしか使わないので依存(read/write)には数えられない
		// (PhaseTag.h の IsQueryOnlyTag 特殊化を参照)。
		//
		//==================================================================================

		template<typename ...Components, typename... Excludes, typename Func>
		void PostDeserializeTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex = {});
		template<typename ...Components, typename... Excludes, typename Func>
		void AwakeTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex = {});
		template<typename ...Components, typename... Excludes, typename Func>
		void StartTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex = {});
		template<typename ...Components, typename... Excludes, typename Func>
		void ActiveTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex = {});
		template<typename ...Components, typename... Excludes, typename Func>
		void ReleaseTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex = {});

		// カスタムタスク(システム内で何度も ForEach を回すとき)。
		// こちらはタグを足さないので、フェーズは第1引数だけで決まる
		template<typename ...Read, typename... Write, typename Func>
		void PostDeserializeCustomTask(ESystemType a_phase, ReadList<Read...>, WriteList<Write...>, Func a_func);
		template<typename ...Read, typename... Write, typename Func>
		void AwakeCustomTask(ESystemType a_phase, ReadList<Read...>, WriteList<Write...>, Func a_func);
		template<typename ...Read, typename... Write, typename Func>
		void StartCustomTask(ESystemType a_phase, ReadList<Read...>, WriteList<Write...>, Func a_func);
		template<typename ...Read, typename... Write, typename Func>
		void ActiveCustomTask(ESystemType a_phase, ReadList<Read...>, WriteList<Write...>, Func a_func);
		template<typename ...Read, typename... Write, typename Func>
		void ReleaseCustomTask(ESystemType a_phase, ReadList<Read...>, WriteList<Write...>, Func a_func);

	protected:

		//==================================================================================
		//
		// 基盤から呼ばれるフック
		//
		//==================================================================================

		/// <summary>生まれたエンティティは必ず PostDeserialize から始める</summary>
		void OnCreateEntitySignature(Signature& a_sig) override;

		/// <summary>構成が変わったエンティティを初期化フェーズへ戻す</summary>
		void OnReenterInitSignature(Signature& a_sig) override;

		/// <summary>PostDeserialize へ入り直すかどうか</summary>
		bool IsReenteringInit(const Signature& a_from, const Signature& a_to) override;

		/// <summary>エンティティの増減・引っ越しがあったので階層の作り直しを促す</summary>
		void OnEntityStructureChanged() override;

		/// <summary>作り直しに回されたエンティティを、後始末を通して初期化へ戻す</summary>
		void RefreshEntities() override;

	private:

		/// <summary>
		/// 解放されるエンティティの子孫にも ReleaseTag を広げる
		/// </summary>
		/// <remarks>
		/// BeginFrame の「引っ越し」が済んだ直後(Release フェーズを走らせる前)に呼ぶこと。
		/// そこなら親のタグが付き終わっているので、同じフレームのうちに
		/// 親子まとめて解放処理を通してから消せる。
		///
		/// 親を消したのに子だけ残ると、宙に浮いたブースターや武器がその場に取り残される。
		/// 消す側(寿命・撃破・エディターの削除)が毎回子を辿るのは書き漏らすので、
		/// 削除の出口が1つしかないここで面倒を見る。
		/// </remarks>
		void PropagateReleaseToChildren();

		/// <summary>ReleaseTag が付いているものを削除予定へ積む</summary>
		void CollectReleasedEntities();
	};

	//======================================================================================
	// テンプレート実装
	//======================================================================================

	template<typename System>
	inline void World::RegisterSystem()
	{
		static_assert(std::is_base_of_v<ISystem, System>, "App::ECS::ISystem を継承していません");

		// システム実体は Init でタスクを登録するだけの入れ物。
		// 実行はタスク側で行うので、基盤へは寿命の保持だけ頼む
		std::shared_ptr<System> _spSys = std::make_shared<System>();
		_spSys->Init(*this);

		HoldSystem(std::move(_spSys));
	}

	template<typename ...Components, typename ...Excludes, typename Func>
	inline void World::PostDeserializeTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex)
	{
		RegisterTask<PostDeserializeTag, Components...>(a_phase, a_taskName, a_func, a_ex);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline void World::AwakeTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex)
	{
		RegisterTask<AwakeTag, Components...>(a_phase, a_taskName, a_func, a_ex);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline void World::StartTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex)
	{
		RegisterTask<StartTag, Components...>(a_phase, a_taskName, a_func, a_ex);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline void World::ActiveTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex)
	{
		RegisterTask<ActiveTag, Components...>(a_phase, a_taskName, a_func, a_ex);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline void World::ReleaseTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex)
	{
		RegisterTask<ReleaseTag, Components...>(a_phase, a_taskName, a_func, a_ex);
	}

	template<typename ...Read, typename ...Write, typename Func>
	inline void World::PostDeserializeCustomTask(ESystemType a_phase, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		RegisterCustomTask(a_phase, ReadList<Read...>{}, WriteList<Write...>{}, a_func);
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline void World::AwakeCustomTask(ESystemType a_phase, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		RegisterCustomTask(a_phase, ReadList<Read...>{}, WriteList<Write...>{}, a_func);
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline void World::StartCustomTask(ESystemType a_phase, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		RegisterCustomTask(a_phase, ReadList<Read...>{}, WriteList<Write...>{}, a_func);
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline void World::ActiveCustomTask(ESystemType a_phase, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		RegisterCustomTask(a_phase, ReadList<Read...>{}, WriteList<Write...>{}, a_func);
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline void World::ReleaseCustomTask(ESystemType a_phase, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		RegisterCustomTask(a_phase, ReadList<Read...>{}, WriteList<Write...>{}, a_func);
	}
}
