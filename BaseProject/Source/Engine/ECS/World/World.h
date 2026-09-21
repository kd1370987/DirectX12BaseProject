#pragma once

#include "../Entity/EntityStorage.h"
#include "../Entity/CommandBuffer.h"
#include "../Archetype/Chunk.h"
#include "../Component/ComponentMetaRegistry.h"
#include "../System/SystemManager.h"
#include "../System/SystemCommon.h"
#include "../Resource/ResourceStore.h"

namespace Engine::ECS
{
	// クエリから除外するコンポーネント
	template<typename... Excludes> struct Exclude {};

	// カスタムタスクの依存(読み込み / 書き込み)
	template<typename... Comps> struct ReadList {};
	template<typename... Comps> struct WriteList {};

	//==========================================================================================
	// ワールド
	//
	// ECS の窓口。エンティティ・コンポーネント・システム・リソースへの操作はすべてここを通す。
	// 実処理は各部品(EntityStorage / CommandBuffer / SystemManager / ResourceStore)が持ち、
	// ワールドは「いつ・どの順で動かすか」を決める。
	//
	// 構造の変更(生成・削除・コンポーネントの付け外し)は反復中にできないので、
	// Reserve* で予約し、BeginFrame(または ApplyReservedChange)でまとめて反映する。
	//
	// ライフサイクルの決めごと(初期化フェーズの進行など)は持たない。
	// それを持つ層(App::ECS::APPWorld)が protected のフックを override する。
	//==========================================================================================
	class World
	{
	public:

		World();
		virtual ~World();

		// コピー・ムーブ禁止(システムやシーンがポインタで持つため)
		World(const World&) = delete;
		World& operator=(const World&) = delete;
		World(World&&) = delete;
		World& operator=(World&&) = delete;

		//==========================================================================================
		// ワールドの寿命
		//==========================================================================================

		// 初期化 : 生成後に1度だけ呼ぶ。
		// 型情報はプロセスに1つ(持ち主は MainEngine)なので、借りてくる
		void Init(ComponentMetaRegistry* a_pComponentRegistry);
		bool IsInit();

		// ゲーム固有の型(コンポーネント / システム)を登録する。
		// サービスとリソースを差し込んだ後に1度だけ呼ばれる。中身は派生が持つ
		virtual void RegisterGameTypes() {}

		// フレームの先頭処理 : 積まれた予約を 生成 → 引っ越し → 削除 → 作り直し の順に反映する
		virtual void BeginFrame();

		// 解放 : エンティティを全部消す(借りているものは解放フックが返す)
		virtual void Release();

		//==========================================================================================
		// エンティティ : 参照
		//==========================================================================================

		// 全エンティティの住所(未使用の枠も含む。pChunk が nullptr なら空き)
		const std::vector<EntityLocation>& GetEntityList();

		// 生存しているエンティティの数
		UINT GetAliveEntityCount();

		// 今このワールドで生きているか。
		// フレームやシーンをまたいで ID を持ち越す側(エディターなど)は、中身を引く前に必ず確かめること
		bool IsAliveEntity(const Entity& a_entity);

		// 住所の取得 : 居なければ空の住所
		const EntityLocation& GetLocation(const Entity& a_entity);

		// 住所からエンティティを引く : 空の住所なら INVALID_ENTITY
		const Entity& GetEntity(const EntityLocation& a_location);

		// GUID からエンティティを探す。
		// 基盤は識別子を持たないので常に INVALID_ENTITY。GUID を持つ層が override する
		virtual Entity GetEntity(const Engine::GUID& a_guid);

		// シグネチャの取得 : 居なければ空
		Signature GetSignature(const Entity& a_entity);

		// コンポーネントを持っているか(未登録の型・居ないエンティティは false)
		template<typename Comp>
		bool HasComponent(const Entity& a_entity);
		bool HasComponent(const Entity& a_entity, const ComponentTypeID& a_comptype);

		//==========================================================================================
		// エンティティ : 生成・削除
		//==========================================================================================

		// 生成の予約(次の BeginFrame で作られる)
		void ReserveCreateEntity(const Signature& a_sig);

		// 初期値付き生成の予約(プレハブの実体化など)
		void ReserveCreateEntityWithData(const Signature& a_sig, ComponentDataMap a_dataMap);

		// その場で作る : 反復(ForEach / システム)の外でだけ呼ぶこと
		Entity CreateEntity(const Signature& a_sig);

		// 解放の予約 : 削除はすべてここを通す。
		// 基盤は次の BeginFrame で消すだけ。後始末を通してから消したい層は override する
		virtual void ReserveReleaseEntity(const Entity& a_entity);

		//==========================================================================================
		// エンティティ : 構成の変更
		//==========================================================================================

		// コンポーネントの付け外しの予約。
		// 追加でデータを渡さなければ既定値(メンバ初期化子)で構築される
		void ReserveAddComponent(ComponentTypeID a_typeID, Entity a_entity, uint8_t* a_pData = nullptr);
		void ReserveRemoveComponent(ComponentTypeID a_typeID, Entity a_entity);

		// シグネチャ変更の予約(付け外しをまとめて行う)
		void ReserveChangeSignature(ChangeEntityCmd a_cmd);

		// 作り直しの予約 : 後始末を通してから初期化をやり直させる(モデルの差し替えなど)
		void ReserveRefreshEntity(const Entity& a_entity);

		// 積まれたシグネチャ変更を今すぐ反映する。
		// 反復が終わった直後に呼べば同じフレームのうちに反映できる。反復中は呼ばないこと
		void ApplyReservedChange();

		// フェーズのタグを Before から After へ張り替える(予約するだけ)
		template<typename Before, typename After>
		void TransitionPhase();

		// 条件付きの張り替え : 述語が false のものは Before のまま残り、次のフレームで再判定される
		template<typename Before, typename After, typename Pred>
		void TransitionPhase(Pred a_canTransition);

		//==========================================================================================
		// コンポーネント : 型情報
		//==========================================================================================

		// 型の登録(付随する処理 ComponentTraits も同時に登録する)
		template<typename Comp>
		ComponentTypeID RegisterComponent(const std::string& a_name);

		// タイプIDの取得 : 未登録なら INVALID_COMPONENTTYPEID
		template<typename Comp>
		ComponentTypeID GetCompTypeID();
		ComponentTypeID GetCompTypeID(const std::string& a_name);

		// メタ情報(サイズ・名前など)
		const ComponentMeta& GetComponentMetaData(const ComponentTypeID& a_typeID);
		const std::vector<ComponentMeta>& GetAllComponentMetaData() const;	// 添え字がタイプID

		// シグネチャに立っているコンポーネントの名前一覧(保存用。名前が保存データのキー)
		std::vector<std::string> GetComponentNames(const Signature& a_sig) const;

		// 付随する処理(構築・保存・編集・解放)
		template<typename Comp>
		const ComponentFunc& GetCompFunc() const;
		const ComponentFunc& GetCompFunc(const ComponentTypeID& a_typeID) const;

		//==========================================================================================
		// コンポーネント : データ
		//==========================================================================================

		// 単体の参照 : 持っていなければ nullptr
		template<typename Comp>
		Comp* RefData(const Entity& a_entity);

		// 単体の参照(バイト列) : 持っていなければ nullptr
		uint8_t* NRefData(const Entity& a_entity, const ComponentTypeID& a_typeID);

		// チャンク内の配列の先頭 : 持っていなければ nullptr
		template<typename Comp>
		Comp* GetComponentArray(Chunk* a_chunk);

		//==========================================================================================
		// クエリ
		//
		// 指定したコンポーネントをすべて持つチャンクごとに
		//   a_func(Chunk*, uint32_t 要素数, Components*...)
		// を呼ぶ。const を付けた型は読み込み専用の配列で渡る
		//==========================================================================================

		template<typename... Components, typename Func>
		void ForEach(Func a_func);

		// 除外指定付き : Excludes を1つでも持つものは飛ばす
		template<typename... Components, typename... Excludes, typename Func>
		void ForEachEx(Func a_func, Exclude<Excludes...>);

		//==========================================================================================
		// システム
		//==========================================================================================

		// タスクの登録 : Components を持つチャンクごとに a_func を呼ぶ。
		// const の型は読み込み、それ以外は書き込みとして実行順の依存に数える。
		// a_func は無捕獲のラムダに限る(必要なものは SystemContext から取る)
		template<typename... Components, typename... Excludes, typename Func>
		void RegisterTask(
			ESystemType a_phase,
			const std::string& a_taskName,
			Func a_func,
			Exclude<Excludes...> a_ex = {}
		);

		// カスタムタスクの登録 : 自動ループせず a_func(const SystemContext&) を1回呼ぶ。
		// 中で ForEach を何度も回すときに使う。依存は ReadList / WriteList で宣言する
		template<typename... Read, typename... Write, typename Func>
		void RegisterCustomTask(
			ESystemType a_phase,
			const std::string& a_taskName,
			ReadList<Read...>,
			WriteList<Write...>,
			Func a_func
		);

		// システム実体の寿命を預ける(生成と Init は上位層が済ませてから渡す)
		void HoldSystem(std::shared_ptr<ISystem> a_spSystem) { m_systemManager.Hold(std::move(a_spSystem)); }

		// フェーズのタスクを実行順に回す
		void RunSystem(ESystemType a_type, float a_dt);

		// フェーズごとの実行順(ソート済みのタスク)
		const std::unordered_map<ESystemType, std::vector<SystemTask*>>& GetCompileTaskMap() const;

		//==========================================================================================
		// サービス・リソース
		//
		// サービス : アプリ寿命(シーンをまたぐ)。合成はシーン側が行い、ここへ差し込む
		// リソース : ワールド寿命。型ごとに1つだけ持つ
		//==========================================================================================

		void SetEngineServices(const EngineServices& a_services) { m_engineServices = a_services; }
		EngineServices* RefEngineServices() { return &m_engineServices; }

		// 登録 : すでにあれば何もしない
		template<typename ResourceType, typename... Args>
		void AddResource(Args&&... a_args);

		// 参照 : 無ければ止める
		template<typename ResourceType>
		ResourceType& GetResource();

		template<typename ResourceType>
		bool HasResource() const;

	protected:

		//==========================================================================================
		// 派生へのフック
		//
		// 「生まれた直後に何を載せるか」「消える前に何を走らせるか」といった
		// ライフサイクルの決めごとを持つ層が override する。
		// タイプIDの取得(GetCompTypeID)が非constなので、フックも非constにしてある
		//==========================================================================================

		// 新しく作るエンティティのシグネチャへ初期状態を載せる
		virtual void OnCreateEntitySignature(Signature& a_sig) { (void)a_sig; }

		// 構成が変わったエンティティを初期化からやり直させる
		virtual void OnReenterInitSignature(Signature& a_sig) { (void)a_sig; }

		// 初期化をやり直すか : true なら引っ越しの前に借りているものを全部返させる
		virtual bool IsReenteringInit(const Signature& a_from, const Signature& a_to)
		{
			(void)a_from; (void)a_to; return false;
		}

		// エンティティの増減・引っ越しがあった
		virtual void OnEntityStructureChanged() {}

		// 作り直しの予約を反映する。基盤には作り直しの工程が無いので捨てるだけ
		virtual void ApplyReservedRefresh();

		//==========================================================================================
		// 予約の反映・即時操作
		//
		// 反復中に呼ぶとチャンクの並びが変わるので、BeginFrame / Release の流れからのみ呼ぶ。
		// 即時削除は後始末を通さないので外には出さない(削除の入口は ReserveReleaseEntity)
		//==========================================================================================

		void ApplyReservedCreate();
		void ApplyReservedRemove();

		// 削除の予約 : 後始末を済ませたエンティティを消すための最後の一手
		void ReserveRemoveEntity(const Entity& a_entity);

		// その場で消す / 引っ越す
		void RemoveEntity(const Entity& a_entity);
		void ChangeSignature(const ChangeEntityCmd& a_cmd);

		//==========================================================================================
		// クエリの内部処理
		//==========================================================================================

		// 型の並びからシグネチャを組む。
		// 未登録の型はビットを立てずに false を返す(立てると範囲外で落ちる)。
		// 絞り込み側で false なら、一致するアーキタイプは存在しない
		template<typename... Comps>
		bool BuildSignature(Signature& a_outSig);

		// 条件に一致するチャンクを集める
		template<typename... Components, typename... Excludes>
		std::vector<Chunk*> BuildChunkQuery(Exclude<Excludes...> a_ex = {});

		// キャッシュ付きで集める : アーキタイプの世代が変わっていなければ前回の結果を返す
		template<typename... Components, typename... Excludes>
		const std::vector<Chunk*>& ResolveQuery(QueryCache& a_cache, Exclude<Excludes...> a_ex = {});

		// チャンクごとにコンポーネント配列を揃えて a_invoke(Chunk*, uint32_t, Components*...) を呼ぶ。
		// チャンクを回すループはここにしか書かない
		template<typename... Components, typename Invoke>
		void ForEachChunk(const std::vector<Chunk*>& a_chunkVec, Invoke&& a_invoke);

	protected:

		// ※ 宣言順が生成・破棄の順になる

		EntityStorage			m_storage;					// エンティティ(ID とチャンクの実体)
		SystemManager			m_systemManager;			// システムのタスクと実行順
		ComponentMetaRegistry*	m_pComponentRegistry = nullptr;	// コンポーネントの型情報(借り物。持ち主は MainEngine)
		EngineServices			m_engineServices = {};		// アプリ寿命のサービス(SystemContext で渡す)
		bool					m_isInit = false;			// 初期化済みか
		CommandBuffer			m_commandBuffer;			// 構造変更の予約
		ResourceStore			m_resourceStore;			// ワールド寿命のリソース
	};

	//==============================================================================================
	// エンティティ
	//==============================================================================================

	template<typename Comp>
	inline bool World::HasComponent(const Entity& a_entity)
	{
		return HasComponent(a_entity, ComponentMetaRegistry::GetTypeID<Comp>());
	}

	template<typename Before, typename After>
	inline void World::TransitionPhase()
	{
		TransitionPhase<Before, After>([](Entity) { return true; });
	}

	template<typename Before, typename After, typename Pred>
	inline void World::TransitionPhase(Pred a_canTransition)
	{
		const ComponentTypeID _beforeID = GetCompTypeID<Before>();
		const ComponentTypeID _afterID = GetCompTypeID<After>();

		// どちらかが未登録なら張り替えようがない(ビットを触ると範囲外で落ちる)
		if (!IsValidTypeID(_beforeID) || !IsValidTypeID(_afterID)) return;

		ForEach<Before>(
			[this, _beforeID, _afterID, &a_canTransition](Chunk* a_pChunk, uint32_t a_count, Before*)
			{
				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					const Entity _entity = a_pChunk->entityData[_i];
					if (!a_canTransition(_entity)) continue;

					Signature _sig = GetSignature(_entity);
					_sig.reset(_beforeID);
					_sig.set(_afterID);

					// 反復中なので予約する
					ReserveChangeSignature({ .entity = _entity, .toSig = _sig });
				}
			}
		);
	}

	//==============================================================================================
	// コンポーネント
	//==============================================================================================

	template<typename Comp>
	inline ComponentTypeID World::RegisterComponent(const std::string& a_name)
	{
		return m_pComponentRegistry->RegisterType<Comp>(a_name);
	}

	template<typename Comp>
	inline ComponentTypeID World::GetCompTypeID()
	{
		// 型ごとの置き場所を読むだけ(typeid やハッシュの検索をしない)
		return ComponentMetaRegistry::GetTypeID<Comp>();
	}

	template<typename Comp>
	inline const ComponentFunc& World::GetCompFunc() const
	{
		return GetCompFunc(ComponentMetaRegistry::GetTypeID<Comp>());
	}

	template<typename Comp>
	inline Comp* World::RefData(const Entity& a_entity)
	{
		return reinterpret_cast<Comp*>(NRefData(a_entity, ComponentMetaRegistry::GetTypeID<Comp>()));
	}

	template<typename Comp>
	inline Comp* World::GetComponentArray(Chunk* a_chunk)
	{
		// const 付きの型でも同じ置き場所を読む
		const ComponentTypeID _typeID = ComponentMetaRegistry::GetTypeID<Comp>();
		return reinterpret_cast<Comp*>(m_storage.RefComponentArray(a_chunk, _typeID));
	}

	//==============================================================================================
	// クエリ
	//==============================================================================================

	template<typename... Components, typename Func>
	inline void World::ForEach(Func a_func)
	{
		ForEachEx<Components...>(a_func, Exclude<>{});
	}

	template<typename... Components, typename... Excludes, typename Func>
	inline void World::ForEachEx(Func a_func, Exclude<Excludes...>)
	{
		ForEachChunk<Components...>(BuildChunkQuery<Components...>(Exclude<Excludes...>{}), a_func);
	}

	template<typename... Comps>
	inline bool World::BuildSignature(Signature& a_outSig)
	{
		bool _isAllValid = true;
		(
			[&]()
			{
				const ComponentTypeID _typeID = ComponentMetaRegistry::GetTypeID<Comps>();
				if (IsValidTypeID(_typeID))
				{
					a_outSig.set(_typeID);
				}
				else
				{
					_isAllValid = false;
				}
			}(), ...
		);
		return _isAllValid;
	}

	template<typename... Components, typename... Excludes>
	inline std::vector<Chunk*> World::BuildChunkQuery(Exclude<Excludes...>)
	{
		// 絞り込み側に未登録の型があれば、それを持つエンティティは居ない
		Signature _querySig;
		if (!BuildSignature<Components...>(_querySig)) return {};

		// 除外側の未登録の型は誰も持っていないので無視してよい
		Signature _excludeSig;
		BuildSignature<Excludes...>(_excludeSig);

		return m_storage.MatchingChunkVec(_querySig, _excludeSig);
	}

	template<typename... Components, typename... Excludes>
	inline const std::vector<Chunk*>& World::ResolveQuery(QueryCache& a_cache, Exclude<Excludes...>)
	{
		const uint64_t _generation = m_storage.GetArchetypeGeneration();
		if (a_cache.IsStale(_generation))
		{
			a_cache.chunkVec = BuildChunkQuery<Components...>(Exclude<Excludes...>{});
			a_cache.generation = _generation;
		}
		return a_cache.chunkVec;
	}

	template<typename... Components, typename Invoke>
	inline void World::ForEachChunk(const std::vector<Chunk*>& a_chunkVec, Invoke&& a_invoke)
	{
		for (Chunk* _chunk : a_chunkVec)
		{
			if (!_chunk || _chunk->count == 0) continue;
			a_invoke(_chunk, _chunk->count, GetComponentArray<Components>(_chunk)...);
		}
	}

	//==============================================================================================
	// システム
	//==============================================================================================

	template<typename... Components, typename... Excludes, typename Func>
	inline void World::RegisterTask(
		ESystemType a_phase,
		const std::string& a_taskName,
		Func a_func,
		Exclude<Excludes...>
	)
	{
		// システムは状態を持てない。捕獲を許すと登録時の値がシーンをまたいで残る
		static_assert(
			std::is_convertible_v<Func, void(*)(Chunk*, uint32_t, const SystemContext&, Components*...)>,
			"システムのラムダは無捕獲(ステートレス)にしてください。World などは SystemContext から取得します。"
		);

		SystemTask _task;
		_task.name = a_taskName;

		// const の有無で読み込み / 書き込みに振り分ける
		(
			[&]()
			{
				using _CompType = std::remove_const_t<Components>;

				// 問い合わせ専用のタグは絞り込み条件であってデータではないので、依存に数えない
				if constexpr (!IsQueryOnlyTag_v<_CompType>)
				{
					const ComponentTypeID _typeID = ComponentMetaRegistry::GetTypeID<_CompType>();

					// 型の登録より先にタスクを登録すると未登録のまま来る
					if (!IsValidTypeID(_typeID))
					{
						ENGINE_WARNING("[ECS] %s : 未登録のコンポーネントを依存に含めようとしました (%s)",
							a_taskName.c_str(), std::string(TypeInfo::GetTypeName<_CompType>()).c_str());
						return;
					}

					if constexpr (std::is_const_v<Components>)
					{
						_task.readSig.set(_typeID);
					}
					else
					{
						_task.writeSig.set(_typeID);
					}
				}
			}(), ...
		);

		// World は捕獲せず、実行時に SystemContext から受け取る
		_task.executeFunc = [a_func](SystemTask& a_task, const SystemContext& a_context)
			{
				if (!a_context.pWorld) return;
				World& _world = *a_context.pWorld;

				_world.ForEachChunk<Components...>(
					_world.ResolveQuery<Components...>(a_task.query, Exclude<Excludes...>{}),
					[&](Chunk* a_pChunk, uint32_t a_count, auto... a_data)
					{
						a_func(a_pChunk, a_count, a_context, a_data...);
					}
				);
			};

		m_systemManager.AddSystemTask(a_phase, _task, a_taskName);
	}

	template<typename... Read, typename... Write, typename Func>
	inline void World::RegisterCustomTask(
		ESystemType a_phase,
		const std::string& a_taskName,
		ReadList<Read...>,
		WriteList<Write...>,
		Func a_func
	)
	{
		SystemTask _task;
		_task.name = a_taskName;

		// 型の登録より先に呼ばれて未登録のまま来たものは、依存に数えられない
		if (!BuildSignature<Read...>(_task.readSig) || !BuildSignature<Write...>(_task.writeSig))
		{
			ENGINE_WARNING("[ECS] %s : 未登録のコンポーネントを依存に含めようとしました", a_taskName.c_str());
		}

		_task.executeFunc = [a_func](SystemTask&, const SystemContext& a_context)
			{
				a_func(a_context);
			};

		m_systemManager.AddSystemTask(a_phase, _task, a_taskName);
	}

	//==============================================================================================
	// リソース
	//==============================================================================================

	template<typename ResourceType, typename... Args>
	inline void World::AddResource(Args&&... a_args)
	{
		m_resourceStore.Add<ResourceType>(std::forward<Args>(a_args)...);
	}

	template<typename ResourceType>
	inline ResourceType& World::GetResource()
	{
		return m_resourceStore.Get<ResourceType>();
	}

	template<typename ResourceType>
	inline bool World::HasResource() const
	{
		return m_resourceStore.Has<ResourceType>();
	}
}
