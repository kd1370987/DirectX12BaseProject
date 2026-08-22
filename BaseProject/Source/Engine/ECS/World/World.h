#pragma once

// マネージャー関係
#include "../EntityManager/EntityManager.h"
#include "../SystemManager/SystemManager.h"
#include "../ArchetypeChunkManager/ArchetypeChunkManager.h"
#include "../ArchetypeChunk/ArchetypeChunk.h"
#include "../ResourceTypeManager/ResourceTypeManager.h"
#include "../ResourceWrapper/ResourceWrapper.h"

#include "../ComponentMetaRegistry/ComponentMetaRegistry.h"

#include "../Internal/SystemComon.h"

#include "../../../Application/Components/Tag/SystemPhaseTag/ActiveTag.h"
#include "../../../Application/Components/Tag/SystemPhaseTag/AwakeTag.h"
#include "../../../Application/Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "../../../Application/Components/Tag/SystemPhaseTag/StartTag.h"
#include "../../../Application/Components/Tag/SystemPhaseTag/ReleaseTag.h"

namespace Engine::ECS
{

	template<typename... Excludes>
	struct Exclude {};

	// エンティティの移動用
	struct ChangeEntityCmd
	{
		Entity entity;		// エンティティ
		Signature toSig;	// 変更予定シグネチャ

		// 指定したデータに書き換え
		std::unordered_map<ComponentTypeID, std::vector<uint8_t>> dataMap = {};
	};

	// データ付きエンティティ生成用(プレハブ実体化など)
	// シグネチャで生成し、dataMap のバイト列を各コンポーネントへ流し込む。
	struct CreateEntityWithDataCmd
	{
		Signature sig;
		std::unordered_map<ComponentTypeID, std::vector<uint8_t>> dataMap = {};
	};

	// カスタムタスク用依存情報
	template<typename ...Comp> struct ReadList {};
	template<typename ...Comp> struct WriteList {};

	class World
	{
	public:

		//==========================================================================================
		// 
		// ワールドに対しての操作関連
		// 
		//==========================================================================================
		
		// 初期化
		void Init();	// 生成後に実行
		bool IsInit();	// 初期化されているかどうか

		// 解放時に実行
		void Release();		// 解放処理
		void ClearMemory();	// 任意のリセットしたいタイミング

		// フレームの初めに呼び出す関数
		// シングルフレームで実行したい、生成や破棄、引っ越しを行う
		void BeginFrame();

		/// <summary>
		/// リソースなどのハンドル管理されているものの解放処理
		/// 数フレームに一度だけ実行して、全エンティティのハンドル数をカウントして、ないものは解放していく
		/// </summary>
		void ResourceGC();

		//==========================================================================================
		// 
		// エンティティ関連
		// 
		//==========================================================================================

		/// <summary>
		/// 生存中のエンティティリストを返す
		/// </summary>
		const std::vector<EntityLocation>& GetEntityList();

		/// <summary>
		/// エンティティのロケーションを取得
		/// </summary>
		const EntityLocation& GetLocation(const Entity& a_entity);

		/// <summary>
		/// 生存エンティティ数を返す
		/// </summary>
		UINT GetAliveEntityCount();

		/// <summary>
		/// そのエンティティが今このワールドで生きているか。
		/// フレームやシーンを跨いでIDを保持する側(エディターなど)は、
		/// 中身を引く前にこれで確かめること
		/// </summary>
		bool IsAliveEntity(const Entity& a_entity);

		/// <summary>
		/// ロケーションからエンティティを取得
		/// </summary>
		const Entity& GetEntity(const EntityLocation& a_location);

		/// <summary>
		/// エンティティのシグネチャを取得
		/// </summary>
		Signature GetSignature(const Entity& a_entity);

		// エンティティがコンポーネントを持っているかどうか
		template<typename Comp>
		bool HasComponent(const Entity& a_entity);
		bool HasComponent(const Entity& a_entity,const std::type_index& a_typeid);
		bool HasComponent(const Entity& a_entity,const ComponentTypeID& a_comptype);

		//------------------------------------------------------------------------------------------
		// エンティティの生成
		//------------------------------------------------------------------------------------------

		void AddEntity(const Signature& a_sig);			// コマンド発行
		// データ付き生成コマンド(プレハブ実体化など)。BeginFrameで安全に生成される。
		void AddEntityWithData(const Signature& a_sig, std::unordered_map<ComponentTypeID, std::vector<uint8_t>> a_dataMap);
		Entity CreateEntity(const Signature& a_sig);	// 実体の作成
		void CreateAllEntity();							// 一括作成

		//------------------------------------------------------------------------------------------
		// エンティティの削除
		//------------------------------------------------------------------------------------------

		// フレームの初めにエンティティを削除する
		void RemoveEntityStorage();

		/// <summary>
		/// エンティティに ReleaseTag を付けて解放予約する : 削除はすべてこれを通す
		///
		/// 次の BeginFrame で ActiveTag が外れて Release フェーズが走り、
		/// そのまま削除される。
		/// 借りているものを返してから消えるので、寿命切れの弾やエフェクト、
		/// 撃破された敵、エディターでの削除で各種プールが漏れない。
		///
		/// 解放処理を通さない即時削除(AddRemoveEntity / RemoveEntity)は、
		/// 借りているものを返す機会がないまま消えて漏れるため private にしてある。
		/// </summary>
		void AddReleaseEntity(const Entity& a_entity);

		//------------------------------------------------------------------------------------------
		// エンティティの検索
		//------------------------------------------------------------------------------------------
		Entity GetEntity(const Engine::GUID& a_guid);		// GUIDからの検索

		//------------------------------------------------------------------------------------------
		// エンティティの操作
		//------------------------------------------------------------------------------------------
		// エンティティに対してコンポーネントを操作
		void AddComponent(ComponentTypeID a_typeID,Entity a_entity,uint8_t* a_pData = nullptr);		// 追加
		void SubmitComponent(ComponentTypeID a_typeID,Entity a_entity);		// 削除
		void AddChangeSigCommand(ChangeEntityCmd a_cmd);					// 指定シグネチャに変更するコマンド
		void ChangeSignature(ChangeEntityCmd a_cmd);						// コマンドから実際にアーキタイプを移動させる

		void AddRefreshEntity(const Entity& a_entity);						// リフレッシュ

		//==========================================================================================
		// 
		// コンポーネント関連
		// 
		//==========================================================================================

		// コンポーネントの型情報を登録
		template<typename Comp>
		ComponentTypeID RegisterComponent(const std::string& a_name);			// 関数も同時に登録する

		// コンポーネントIDの取得
		ComponentTypeID GetCompTypeID(const std::type_index& a_index);
		ComponentTypeID GetCompTypeID(const std::string& a_name);
		template<typename Comp>
		ComponentTypeID GetCompTypeID();

		// ネイティブなバイトデータへのポインタを取得
		uint8_t* NRefData(const Entity& a_entity, const std::type_index& a_index);
		uint8_t* NRefData(const Entity& a_entity, const ComponentTypeID& a_typeID);

		/// <summary>
		/// コンポーネントを型として参照取得
		/// </summary>
		/// <typeparam name="Comp">型情報</typeparam>
		/// <param name="a_entity">エンティティID</param>
		template<typename Comp>
		Comp* RefData(const Entity& a_entity);

		/// <summary>
		/// 指定した ArchetypeChunk からテンプレート型 Comp のコンポーネント配列へのポインタを取得します
		/// </summary>
		/// <typeparam name="Comp">取得するコンポーネントの型</typeparam>
		/// <param name="a_chunk">コンポーネント配列を取得する対象の ArchetypeChunk を指すポインタ</param>
		/// <returns>チャンク内の Comp 型コンポーネント配列へのポインタ</returns>
		template<typename Comp>
		Comp* GetComponentArray(ArchetypeChunk* a_chunk);

		// 指定コンポーネントの情報を取得
		const ComponentMeta& GetComponentMetaData(const ComponentTypeID& a_typeID);	// メタデータ
		const ComponentFunc& GetCompFunc(const ComponentTypeID& a_typeID)const;		// 関数
		template<typename Comp>
		const ComponentFunc& GetCompFunc()const;		// 関数

		// 全コンポーネントの情報を取得
		const std::unordered_map<ComponentTypeID, ComponentMeta>& GetAllComponentMetaData() const;

		//==========================================================================================
		// 
		// システム関連
		// 
		//==========================================================================================

		// システムの登録
		template<typename System>
		void RegisterSystem();

		// システムの実行
		// システムフェーズを指定してデルタタイムを渡す
		void RunSystem(ESystemType a_type, float a_dt);

		// アプリ寿命のサービス群を差し込む(合成はシーン側が行う)
		void SetEngineServices(const EngineServices& a_services) { m_engineServices = a_services; }
		EngineServices* RefEngineServices() { return &m_engineServices; }

		// 登録済みタスクの実行本体
		// 実行時に渡された SystemContext の World に対してクエリを回す。
		// 登録時の World を捕獲しないため、同じシステムを別 World へも流せる。
		template<typename ...Components, typename... Excludes, typename Func>
		void DispatchTask(const SystemContext& a_context, Func a_func, Exclude<Excludes...> a_ex = {});

		// 収集関数
		// 指定したコンポーネント群を持つすべてのチャンクに対して、指定された関数を実行します
		template<typename... Components, typename Func>
		void ForEach(Func a_func);

		// 除外指定付き取集関数
		// 除外コンポーネントをテンプレートで第二引数で受け取る
		template<typename... Components, typename... Excludes, typename Func>
		void ForEachEx(Func a_func, Exclude<Excludes...>);

		// システムのフェーズ遷移
		template<typename Before,typename After>
		void TransitionPhase();

		/// <summary>
		/// 条件付きのフェーズ遷移
		/// 述語が false を返したエンティティは Before のタグを持ったまま残る。
		/// フェーズは毎フレーム回るので、次のフレームで再判定される
		/// </summary>
		/// <param name="a_canTransition">Entity を受け取り、進めてよいなら true</param>
		template<typename Before, typename After, typename Pred>
		void TransitionPhase(Pred a_canTransition);

		// コンパイルされたパスの取得
		const std::unordered_map<ESystemType, std::vector<SystemTask*>>& GetCompileTaskMap()const;

		//------------------------------------------------------------------------------------------
		// システムタスクの登録
		//------------------------------------------------------------------------------------------
		// 静的に式を保存して呼び出す
		template<typename ...Components, typename... Excludes, typename Func>
		void RegisterTask(
			ESystemType a_phase, 
			const std::string& a_taskName, 
			Func a_func, 
			Exclude<Excludes...> a_ex = {}
		);

		// フェーズごとの専用登録関数
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

		// カスタムタスク登録
		// システム内で何度もForEachなどを使うときに使用
		template<typename ...Read, typename... Write, typename Func>
		void RegisterCustomTask(ESystemType a_phase, ReadList<Read...>,WriteList<Write...>,Func a_func);

		// フェーズごとの専用関数
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

		//==========================================================================================
		// 
		// リソース関連
		// 
		//==========================================================================================

		// リソースの登録
		template<typename ResourceType,typename... Args>
		void AddResource(Args&&... a_args);

		// リソースの参照
		template<typename ResourceType>
		ResourceType& GetResource();

		// リソース生存チェック
		template<typename ResourceType>
		bool HasResource() const;

	private:

		// リフレッシュリストにたまったエンティティを一括で処理する
		void RefreshEntities();

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

		//------------------------------------------------------------------------------------------
		// エンティティの即時削除
		//
		// Release フェーズを通さずに消すので、外からは使わせない。
		// 解放処理を終えたエンティティを実際に片付ける最後の一手として、
		// BeginFrame / Release からのみ呼ぶこと。削除の入口は AddReleaseEntity。
		//------------------------------------------------------------------------------------------

		// 削除予定エンティティを追加
		void AddRemoveEntity(const Entity& a_entity);

		// エンティティの削除
		void RemoveEntity(const Entity& a_entity);

	private:

		// マネージャー軍
		EntityManager m_entityManager;
		SystemManager m_systemManager;
		ArchetypeChunkManager m_archetypeChunkManager;

		// コンポーネントメタ情報管理
		ComponentMetaRegistry m_componentMetaRegistry;

		// アプリ寿命のサービス群(SystemContext 経由でシステムへ渡す)
		EngineServices m_engineServices = {};

		// 初期化済み
		bool m_isInit = false;

		// 生成予定エンティティリスト
		std::vector<Signature> m_addEntityVec = {};

		// データ付き生成予定エンティティリスト(プレハブ実体化など)
		std::vector<CreateEntityWithDataCmd> m_addEntityDataVec = {};

		// 削除予定エンティティ
		std::vector<Entity> m_removeEntityVec = {};

		// 移動予定エンティティ
		std::vector<ChangeEntityCmd> m_changeEntityVec = {};

		// リフレッシュ予定エンティティ
		std::vector<Entity> m_refreshEntityVec = {};

		// インターフェースポインタでリソースを保存
		std::unordered_map<ResourceTypeID, std::unique_ptr<IResourceWrapper>> m_resourceMap;

	public:
		// コンストラクタデストラクタ
		World();
		~World();

		// コピー禁止
		World(const World&) = delete;
		World& operator=(const World&) = delete;

		// ムーブ禁止
		World(World&&) = delete;
		World& operator = (World&&) = delete;
	};

	template<typename Comp>
	inline bool World::HasComponent(const Entity& a_entity)
	{
		return HasComponent(a_entity,typeid(Comp));
	}

	template<typename Comp>
	inline ComponentTypeID World::RegisterComponent(const std::string& a_name)
	{
		auto _id = m_componentMetaRegistry.RegisterType<Comp>(a_name);
		return _id;
	}

	template<typename Comp>
	inline ComponentTypeID World::GetCompTypeID()
	{
		return GetCompTypeID(typeid(Comp));
	}

	template<typename Comp>
	inline Comp* World::RefData(const Entity& a_entity)
	{
		return reinterpret_cast<Comp*>(
			NRefData(a_entity, typeid(Comp))
			);
	}

	template<typename Comp>
	inline Comp* World::GetComponentArray(ArchetypeChunk* a_chunk)
	{
		// タイプIDの取得はconst を外した純粋な型で行う
		using RawType = std::remove_const_t<Comp>;
		auto _typeID = m_componentMetaRegistry.GetTypeID<RawType>();

		return reinterpret_cast<Comp*>(m_archetypeChunkManager.RefComponentArray(a_chunk, _typeID));
	}

	template<typename Comp>
	inline const ComponentFunc& World::GetCompFunc() const
	{
		auto _id = m_componentMetaRegistry.GetTypeID(typeid(Comp));
		return GetCompFunc(_id);
	}

	template<typename System>
	inline void World::RegisterSystem()
	{
		m_systemManager.Register<System>(this);
	}

	template<typename ...Components, typename Func>
	inline void World::ForEach(Func a_func)
	{
		// シグネチャを生成
		Signature _sig;
		(_sig.set(m_componentMetaRegistry.GetTypeID<Components>()), ...);

		// チャンクの配列を取得
		for (auto* _chunk : m_archetypeChunkManager.MatchingArchetypeChunkVec(_sig))
		{
			if (!_chunk || _chunk->count == 0) continue;

			// 操作しやすいように配列にして返す
			auto _arrays = std::forward_as_tuple(
				GetComponentArray<Components>(_chunk)...
			);

			std::apply(
				[&](auto... a_data)
				{
					a_func(_chunk, _chunk->count, a_data...);
				},
				_arrays
			);
		}
	}

	template<typename ...Components, typename ...Excludes, typename Func>
	inline void World::ForEachEx(Func a_func, Exclude<Excludes...>)
	{
		// シグネチャを生成
		Signature _sig;
		(_sig.set(m_componentMetaRegistry.GetTypeID<Components>()), ...);
		Signature _excludeSig;
		(_excludeSig.set(m_componentMetaRegistry.GetTypeID<Excludes>()), ...);
		// チャンクの配列を取得
		for (auto* _chunk : m_archetypeChunkManager.MatchingArchetypeChunkVecEx(_sig, _excludeSig))
		{
			if (!_chunk || _chunk->count == 0) continue;
			// 操作しやすいように配列にして返す
			auto _arrays = std::forward_as_tuple(
				GetComponentArray<Components>(_chunk)...
			);
			std::apply(
				[&](auto... a_data)
				{
					a_func(_chunk, _chunk->count, a_data...);
				},
				_arrays
			);
		}
	}

	template<typename Before, typename After>
	inline void World::TransitionPhase()
	{
		// 条件なし : すべて進める
		TransitionPhase<Before, After>([](Entity) { return true; });
	}

	template<typename Before, typename After, typename Pred>
	inline void World::TransitionPhase(Pred a_canTransition)
	{
		// コンポーネントのタイプIDを取得
		ComponentTypeID _befforID = GetCompTypeID<Before>();
		ComponentTypeID _affterID = GetCompTypeID<After>();

		ForEach<Before>(
			[this,_befforID,_affterID,&a_canTransition]
			(
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				Before* a_compArray
			)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					// シグネチャの取得
					Entity _entity = a_pChunk->entityData[_i];

					// まだ進めないものは Before のタグを持ったまま残す。
					// フェーズは毎フレーム回るので、次のフレームで再度判定される
					if (!a_canTransition(_entity)) continue;

					Signature _sig = GetSignature(_entity);

					// シグネチャに対してBeforeIDを排除してAfterを入れる
					_sig.reset(_befforID);
					_sig.set(_affterID);

					// 変更予定エンティティとしてリストに追加
					ChangeEntityCmd _cmd = {};
					_cmd.entity = _entity;
					_cmd.toSig = _sig;
					AddChangeSigCommand(_cmd);
				}
			}
		);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline void World::RegisterTask(
		ESystemType a_phase,
		const std::string& a_taskName,
		Func a_func,
		Exclude<Excludes...> a_ex
	)
	{
		SystemTask _task;
		_task.name = a_taskName;
		// テンプレート引数から、Read/Write のシグネチャを分離して生成
		// 関数を作って、テンプレート数分回す
		(
			[&]()
			{
				using _CompType = std::remove_const_t<Components>;

				// フェーズタグ(ActiveTag など)は「データ」ではなく問い合わせ条件なので、
				// 実行順を決める依存(read/write)には含めない。
				//
				// 含めてしまうと、ActiveTask は先頭に ActiveTag を非constで足すので
				// 「全ての ActiveTask が ActiveTag の書き手」になる。一方 ActiveCustomTask は
				// ActiveTag を読み手として持つため、カスタムタスクが書いた成分を読む
				// ActiveTask が1つでも現れた瞬間に相互依存(循環)が成立して
				// トポロジカルソートが失敗する。タグは誰も書き換えないので外すのが正しい。
				//
				// 問い合わせ用のシグネチャは DispatchTask が Components... から作り直すので、
				// ここで外してもタグによる絞り込みは効いたまま。
				constexpr bool _isPhaseTag =
					std::is_same_v<_CompType, ActiveTag> ||
					std::is_same_v<_CompType, StartTag> ||
					std::is_same_v<_CompType, AwakeTag> ||
					std::is_same_v<_CompType, PostDeserializeTag> ||
					std::is_same_v<_CompType, ReleaseTag>;

				if constexpr (!_isPhaseTag)
				{
					// const がついていたら読み込み用
					// const を外した元の型でTypeIDを取得
					auto _typeID = m_componentMetaRegistry.GetTypeID<_CompType>();

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

		// システムは状態を持てない(無捕獲ラムダのみ許可)。
		// 捕獲を許すと登録時の値がシーンをまたいで残り、追いにくい不具合になる。
		// 必要な参照は SystemContext から取ること。
		static_assert(
			std::is_convertible_v<
				Func,
				void(*)(ArchetypeChunk*, uint32_t, const SystemContext&, Components*...)
			>,
			"システムのラムダは無捕獲(ステートレス)にしてください。World などは SystemContext から取得します。"
		);

		// 実行ロジックをラムダ式に包んでタスクとして保存。
		// World は捕獲せず、実行時に SystemContext から受け取る。
		_task.executeFunc = [a_func](const SystemContext& a_context)
			{
				if (!a_context.pWorld) return;
				a_context.pWorld->DispatchTask<Components...>(a_context, a_func, Exclude<Excludes...>{});
			};

		m_systemManager.AddSystemTask(a_phase, _task,a_taskName);
	}

	template<typename ...Components, typename ...Excludes, typename Func>
	inline void World::DispatchTask(const SystemContext& a_context, Func a_func, Exclude<Excludes...>)
	{
		// 実行用のシグネチャ
		Signature _querySig;
		(_querySig.set(m_componentMetaRegistry.GetTypeID<std::remove_const_t<Components>>()), ...);
		Signature _excludeSig;
		(_excludeSig.set(m_componentMetaRegistry.GetTypeID<Excludes>()), ...);

		// チャンクの配列を取得
		for (auto* _chunk : m_archetypeChunkManager.MatchingArchetypeChunkVecEx(_querySig, _excludeSig))
		{
			if (!_chunk || _chunk->count == 0) continue;
			// 操作しやすいように配列にして返す
			auto _arrays = std::forward_as_tuple(
				GetComponentArray<Components>(_chunk)...
			);
			std::apply(
				[&](auto... a_data)
				{
					a_func(_chunk, _chunk->count, a_context, a_data...);
				},
				_arrays
			);
		}
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
		RegisterTask<StartTag, Components...>(a_phase, a_taskName,  a_func,a_ex);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline void World::ActiveTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex)
	{
		RegisterTask<ActiveTag, Components...>(a_phase, a_taskName,a_func, a_ex);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline void World::ReleaseTask(ESystemType a_phase, const std::string& a_taskName, Func a_func, Exclude<Excludes...> a_ex)
	{
		RegisterTask<ReleaseTag, Components...>(a_phase, a_taskName, a_func, a_ex);
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline void World::RegisterCustomTask(ESystemType a_phase, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		SystemTask _task;

		// ReadList から読み込みシグネチャを生成
		if constexpr (sizeof...(Read) > 0)
		{
			(_task.readSig.set(m_componentMetaRegistry.GetTypeID<Read>()), ...);
		}

		// WriteList から書き込みシグネチャを生成
		if constexpr (sizeof...(Write) > 0)
		{
			(_task.writeSig.set(m_componentMetaRegistry.GetTypeID<Write>()), ...);
		}
		// 実行関数は自動ループせず、そのまま登録する
		_task.executeFunc = [a_func](const SystemContext& a_context)
			{
				a_func(a_context);
			};

		m_systemManager.AddSystemTask(a_phase, _task,"CatamTask");
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline void World::PostDeserializeCustomTask(ESystemType a_phase, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		RegisterCustomTask(
			a_phase,
			ReadList<Read...>{},
			WriteList<Write...>{},
			a_func
		);
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline void World::AwakeCustomTask(ESystemType a_phase, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		RegisterCustomTask(
			a_phase,
			ReadList<Read...>{},
			WriteList<Write...>{},
			a_func
		);
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline void World::StartCustomTask(ESystemType a_phase, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		RegisterCustomTask(
			a_phase,
			ReadList<Read...>{},
			WriteList<Write...>{},
			a_func
		);
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline void World::ActiveCustomTask(ESystemType a_phase, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		RegisterCustomTask(
			a_phase,
			ReadList<Read...>{},
			WriteList<Write...>{},
			a_func
		);
	}
	template<typename ...Read, typename ...Write, typename Func>
	inline void World::ReleaseCustomTask(ESystemType a_phase, ReadList<Read...>, WriteList<Write...>, Func a_func)
	{
		RegisterCustomTask(
			a_phase,
			ReadList<Read...>{},
			WriteList<Write...>{},
			a_func
		);
	}
	template<typename ResourceType, typename ...Args>
	inline void World::AddResource(Args && ...a_args)
	{
		// ID取得
		ResourceTypeID _id = ResourceTypeManager::GetID<ResourceType>();

		if (m_resourceMap.find(_id) == m_resourceMap.end())
		{
			// unique_ptrを使って安全にアップキャストして保持
			// ランタイム中では行わずに初期登録時のみ走る
			m_resourceMap.emplace(_id, std::make_unique<ResourceWrapper<ResourceType>>(std::forward<Args>(a_args)...));
		}
	}
	template<typename ResourceType>
	inline ResourceType& World::GetResource()
	{
		// IDを検索
		ResourceTypeID _id = ResourceTypeManager::GetID<ResourceType>();
		auto _it = m_resourceMap.find(_id);

		// 見つからなければエラー
		if(_it == m_resourceMap.end())
		{
			Editor::MainEditor::Instance().ErrorLog("ECS::World : Resource not found");
		}

		// RTTIによる型チェックを行わずに型が一致している前提でキャスト
		auto* _wrapper = static_cast<ResourceWrapper<ResourceType>*>(_it->second.get());
		return _wrapper->data;
	}
	template<typename ResourceType>
	inline bool World::HasResource() const
	{
		// IDを検索してマップ内に存在するかどうかを返す
		ResourceTypeID _id = ResourceTypeManager::GetID<ResourceType>();
		return m_resourceMap.find(_id) != m_resourceMap.end();
	}
}