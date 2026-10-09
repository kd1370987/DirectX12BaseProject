#pragma once
//==========================================================================================
//
// App::ECS::APPWorld
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
#include "../ISystem/APPISystem.h"

namespace App::ECS
{
	// 基盤の型をそのまま使う
	using Entity			= Engine::ECS::Entity;
	using Signature			= Engine::ECS::Signature;
	using ComponentTypeID	= Engine::ECS::ComponentTypeID;
	using ESystemType		= Engine::ECS::ESystemType;
	using Chunk	= Engine::ECS::Chunk;
	using SystemContext		= Engine::ECS::SystemContext;
	using ChangeEntityCmd	= Engine::ECS::ChangeEntityCmd;
	using ETaskExec			= Engine::ECS::ETaskExec;

	template<typename... T> using Exclude	= Engine::ECS::Exclude<T...>;
	template<typename... T> using ReadList	= Engine::ECS::ReadList<T...>;
	template<typename... T> using WriteList	= Engine::ECS::WriteList<T...>;
	using TaskAccess		= Engine::ECS::TaskAccess;

	class APPWorld : public Engine::ECS::World
	{
	public:

		using Base = Engine::ECS::World;

		// 自分が使うシングルトンリソースはここで確保する
		// (BeginFrame が毎フレーム引くので、無いと成立しない)
		APPWorld();

		//==================================================================================
		//
		// 基盤から差し替えるもの
		//
		//==================================================================================

		/// <summary>フレームの先頭処理 : 初期化フェーズを1フレームで通しきる</summary>
		void BeginFrame() override;

		/// <summary>解放 : 後始末のフェーズを通してから全部消す</summary>
		void Release() override;

		/// <summary>まだ動き出していないエンティティの数</summary>
		/// <remarks>
		/// PostDeserializeTag / AwakeTag / StartTag のどれかを持つもの。
		/// リソースが届くまで Awake で待たされているものもここに数える
		/// </remarks>
		uint32_t GetPendingStartCount() override;

		/// <summary>
		/// エンティティに ReleaseTag を付けて解放予約する : 削除はすべてこれを通す
		/// </summary>
		/// <remarks>
		/// 次の BeginFrame で ActiveTag が外れて Release フェーズが走り、そのまま削除される。
		/// 借りているものを返してから消えるので、寿命切れの弾やエフェクト、
		/// 撃破された敵、エディターでの削除で各種プールが漏れない。
		/// </remarks>
		void ReserveReleaseEntity(const Entity& a_entity) override;

		/// <summary>GUIDからエンティティを探す</summary>
		/// <remarks>
		/// 索引(GUID → Entity)を引く。構造変更があったフレームは最初の呼び出しで作り直す。
		/// 索引が引けない・古いときは全件を舐めて探す(以前と同じ結果になる)。
		/// 索引を書き換えるので、メインスレッドからだけ呼ぶこと(ジョブタスクの中では呼ばない)
		/// </remarks>
		Entity GetEntity(const Core::GUID& a_guid) override;

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
		// 戻り値(TaskAccess)で、絞り込みに使わない読み書きを追加で宣言できる。
		// タグは絞り込みにしか使わないので依存(read/write)には数えられない
		// (PhaseTag.h の IsQueryOnlyTag 特殊化を参照)。
		//
		//==================================================================================

		template<typename ...Components, typename... Excludes, typename Func>
		TaskAccess PostDeserializeTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex = {});
		template<typename ...Components, typename... Excludes, typename Func>
		TaskAccess AwakeTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex = {});
		template<typename ...Components, typename... Excludes, typename Func>
		TaskAccess StartTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex = {});
		template<typename ...Components, typename... Excludes, typename Func>
		TaskAccess ActiveTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex = {});
		template<typename ...Components, typename... Excludes, typename Func>
		TaskAccess ReleaseTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex = {});

		// カスタムタスク(システム内で何度も ForEach を回すとき)。
		// こちらはタグを足さないので、フェーズは第1引数だけで決まる
		template<typename ...Read, typename... Write, typename Func>
		TaskAccess PostDeserializeCustomTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func);
		template<typename ...Read, typename... Write, typename Func>
		TaskAccess AwakeCustomTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func);
		template<typename ...Read, typename... Write, typename Func>
		TaskAccess StartCustomTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func);
		template<typename ...Read, typename... Write, typename Func>
		TaskAccess ActiveCustomTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func);
		template<typename ...Read, typename... Write, typename Func>
		TaskAccess ReleaseCustomTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func);

		//==================================================================================
		//
		// フェーズごとのジョブタスク登録
		//
		// 上の各タスクと同じ登録を、ワーカースレッドで走らせる指定で行う。
		// 自分とぶつかる(読み書きが重なる)タスクの直前まで待ち合わせないので、
		// 間にある無関係なタスクと並行して進む。フェーズの終わりでは必ず待ち合わせる。
		//
		// ワーカーで走るため、中では次のことをしないこと
		//   ・宣言していないコンポーネントへの読み書き
		//   ・構造変更の予約(ReserveReleaseEntity / AddComponent 等)、リソースの追加
		//   ・サービス(デバッグ描画・オーディオ・物理への submit 等)の呼び出し
		//   ・ジョブの発行や完了待ち(子ジョブより先に自分が完了扱いになる / デッドロックする)
		//
		//==================================================================================

		template<typename ...Components, typename... Excludes, typename Func>
		TaskAccess PostDeserializeJobTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex = {});
		template<typename ...Components, typename... Excludes, typename Func>
		TaskAccess AwakeJobTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex = {});
		template<typename ...Components, typename... Excludes, typename Func>
		TaskAccess StartJobTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex = {});
		template<typename ...Components, typename... Excludes, typename Func>
		TaskAccess ActiveJobTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex = {});
		template<typename ...Components, typename... Excludes, typename Func>
		TaskAccess ReleaseJobTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex = {});

		template<typename ...Read, typename... Write, typename Func>
		TaskAccess PostDeserializeCustomJobTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func);
		template<typename ...Read, typename... Write, typename Func>
		TaskAccess AwakeCustomJobTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func);
		template<typename ...Read, typename... Write, typename Func>
		TaskAccess StartCustomJobTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func);
		template<typename ...Read, typename... Write, typename Func>
		TaskAccess ActiveCustomJobTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func);
		template<typename ...Read, typename... Write, typename Func>
		TaskAccess ReleaseCustomJobTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func);

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
		void ApplyReservedRefresh() override;

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

		/// <summary>GUID の索引を全件から作り直す</summary>
		void RebuildGuidIndex();

		/// <summary>全件を舐めて GUID を探す(索引が使えないとき)</summary>
		Entity FindEntityByScan(const Core::GUID& a_guid);

		/// <summary>索引の答えが今も正しいか(生きていて、同じ GUID を持っている)</summary>
		bool IsGuidIndexEntryValid(const Entity& a_entity, const Core::GUID& a_guid);

	private:

		// GUID → Entity の索引。構造変更(生成・削除・引っ越し)で古くなったら作り直す。
		// GUID は PostDeserialize(GUIDFixupSystem)でしか書き換わらず、その後には必ず構造変更が入る
		std::unordered_map<Core::GUID, Entity> m_guidIndexMap = {};
		bool m_isGuidIndexDirty = true;
	};

	//======================================================================================
	// テンプレート実装
	//======================================================================================

	template<typename System>
	inline void APPWorld::RegisterSystem()
	{
		static_assert(std::is_base_of_v<APPISystem, System>, "App::ECS::APPISystem を継承していません");

		// システム実体は Init でタスクを登録するだけの入れ物。
		// 実行はタスク側で行うので、基盤へは寿命の保持だけ頼む
		std::unique_ptr<System> _upSys = std::make_unique<System>();
		_upSys->Init(*this);

		HoldSystem(std::move(_upSys));
	}

	template<typename ...Components, typename ...Excludes, typename Func>
	inline TaskAccess APPWorld::PostDeserializeTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex)
	{
		return RegisterTask<Component::PostDeserializeTag, Components...>(a_phase, a_taskName, a_func, a_ex);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline TaskAccess APPWorld::AwakeTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex)
	{
		return RegisterTask<Component::AwakeTag, Components...>(a_phase, a_taskName, a_func, a_ex);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline TaskAccess APPWorld::StartTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex)
	{
		return RegisterTask<Component::StartTag, Components...>(a_phase, a_taskName, a_func, a_ex);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline TaskAccess APPWorld::ActiveTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex)
	{
		return RegisterTask<Component::ActiveTag, Components...>(a_phase, a_taskName, a_func, a_ex);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline TaskAccess APPWorld::ReleaseTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex)
	{
		return RegisterTask<Component::ReleaseTag, Components...>(a_phase, a_taskName, a_func, a_ex);
	}

	template<typename ...Read, typename ...Write, typename Func>
	inline TaskAccess APPWorld::PostDeserializeCustomTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		return RegisterCustomTask(a_phase, a_taskName, ReadList<Read...>{}, WriteList<Write...>{}, a_func);
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline TaskAccess APPWorld::AwakeCustomTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		return RegisterCustomTask(a_phase, a_taskName, ReadList<Read...>{}, WriteList<Write...>{}, a_func);
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline TaskAccess APPWorld::StartCustomTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		return RegisterCustomTask(a_phase, a_taskName, ReadList<Read...>{}, WriteList<Write...>{}, a_func);
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline TaskAccess APPWorld::ActiveCustomTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		return RegisterCustomTask(a_phase, a_taskName, ReadList<Read...>{}, WriteList<Write...>{}, a_func);
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline TaskAccess APPWorld::ReleaseCustomTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		return RegisterCustomTask(a_phase, a_taskName, ReadList<Read...>{}, WriteList<Write...>{}, a_func);
	}

	//--------------------------------------------------------------------------------------
	// ジョブタスク
	//--------------------------------------------------------------------------------------

	template<typename ...Components, typename ...Excludes, typename Func>
	inline TaskAccess APPWorld::PostDeserializeJobTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex)
	{
		return RegisterTask<Component::PostDeserializeTag, Components...>(a_phase, a_taskName, a_func, a_ex, ETaskExec::Job);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline TaskAccess APPWorld::AwakeJobTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex)
	{
		return RegisterTask<Component::AwakeTag, Components...>(a_phase, a_taskName, a_func, a_ex, ETaskExec::Job);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline TaskAccess APPWorld::StartJobTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex)
	{
		return RegisterTask<Component::StartTag, Components...>(a_phase, a_taskName, a_func, a_ex, ETaskExec::Job);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline TaskAccess APPWorld::ActiveJobTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex)
	{
		return RegisterTask<Component::ActiveTag, Components...>(a_phase, a_taskName, a_func, a_ex, ETaskExec::Job);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline TaskAccess APPWorld::ReleaseJobTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex)
	{
		return RegisterTask<Component::ReleaseTag, Components...>(a_phase, a_taskName, a_func, a_ex, ETaskExec::Job);
	}

	template<typename ...Read, typename ...Write, typename Func>
	inline TaskAccess APPWorld::PostDeserializeCustomJobTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		return RegisterCustomTask(a_phase, a_taskName, ReadList<Read...>{}, WriteList<Write...>{}, a_func, ETaskExec::Job);
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline TaskAccess APPWorld::AwakeCustomJobTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		return RegisterCustomTask(a_phase, a_taskName, ReadList<Read...>{}, WriteList<Write...>{}, a_func, ETaskExec::Job);
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline TaskAccess APPWorld::StartCustomJobTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		return RegisterCustomTask(a_phase, a_taskName, ReadList<Read...>{}, WriteList<Write...>{}, a_func, ETaskExec::Job);
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline TaskAccess APPWorld::ActiveCustomJobTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		return RegisterCustomTask(a_phase, a_taskName, ReadList<Read...>{}, WriteList<Write...>{}, a_func, ETaskExec::Job);
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline TaskAccess APPWorld::ReleaseCustomJobTask(ESystemType a_phase, const std::string& a_taskName, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		return RegisterCustomTask(a_phase, a_taskName, ReadList<Read...>{}, WriteList<Write...>{}, a_func, ETaskExec::Job);
	}
}
