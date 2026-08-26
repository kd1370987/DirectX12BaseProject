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

		//----------------------------------------------------------------------------------
		// ゲーム固有の型(コンポーネント / システム)を登録する
		//
		// 呼ぶ場所は基盤(シーンのワールド生成)が決め、中身は派生が持つ。
		// サービスとリソースを差し込んだ後に一度だけ呼ばれる。
		//----------------------------------------------------------------------------------
		virtual void RegisterGameTypes() {}

		// 解放時に実行
		// 解放処理 : エンティティを全部消す
		// (コンポーネントが借りているリソースは解放フックが返す)
		//
		// 基盤は「全部消す」だけ。消す前に何かを走らせたい層は override すること
		virtual void Release();
		void ClearMemory();	// 任意のリセットしたいタイミング

		// フレームの初めに呼び出す関数
		// シングルフレームで実行したい、生成や破棄、引っ越しを行う
		//
		// 基盤がやるのは「積まれた命令を捌く」ところまで。
		// ライフサイクル(初期化フェーズの進行)を持つ層は override して差し込む
		virtual void BeginFrame();

		/// <summary>
		/// コンポーネントが借りているものを返させる
		/// </summary>
		/// <remarks>
		/// コンポーネントは trivially copyable 縛りでデストラクタが走らないので、
		/// リソースの参照カウントのように「取ったら返す」ものは
		/// ComponentTraits<T>::Release に書き、ここから呼ばせる。
		/// 呼ぶのはエンティティを消すときと、コンポーネントを外す/入れ直すとき。
		/// </remarks>
		void ReleaseComponents(const ECS::Entity& a_entity, const Signature& a_sig);

		/// <summary>
		/// 退避したコンポーネントのデータに対して解放フックを呼ぶ
		/// (アーキタイプの引っ越し中は実体の置き場所が変わるため)
		/// </summary>
		void ReleaseComponentData(ComponentTypeID a_compID, uint8_t* a_pData);


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
		/// エンティティの解放を予約する : 削除はすべてこれを通す
		/// </summary>
		/// <remarks>
		/// 基盤は次の BeginFrame で消すだけ。
		/// 消える前に後始末を走らせたい層(App::ECS::World)は override して、
		/// 解放フェーズを通してから消えるようにしている。
		///
		/// 解放処理を通さない即時削除(AddRemoveEntity / RemoveEntity)は、
		/// 借りているものを返す機会がないまま消えて漏れるため protected にしてある。
		/// </remarks>
		virtual void AddReleaseEntity(const Entity& a_entity);

		//------------------------------------------------------------------------------------------
		// エンティティの検索
		//------------------------------------------------------------------------------------------
		/// <summary>GUIDからエンティティを探す</summary>
		/// <remarks>
		/// 基盤はエンティティに識別子を持たせないので、常に INVALID_ENTITY を返す。
		/// GUIDを持つコンポーネントを定義した層が override する
		/// </remarks>
		virtual Entity GetEntity(const Engine::GUID& a_guid);

		//------------------------------------------------------------------------------------------
		// エンティティの操作
		//------------------------------------------------------------------------------------------
		// エンティティに対してコンポーネントを操作
		void AddComponent(ComponentTypeID a_typeID,Entity a_entity,uint8_t* a_pData = nullptr);		// 追加
		void SubmitComponent(ComponentTypeID a_typeID,Entity a_entity);		// 削除
		void AddChangeSigCommand(ChangeEntityCmd a_cmd);					// 指定シグネチャに変更するコマンド
		void ChangeSignature(ChangeEntityCmd a_cmd);						// コマンドから実際にアーキタイプを移動させる

		/// <summary>
		/// 溜まっているシグネチャ変更を今すぐ反映する
		/// </summary>
		/// <remarks>
		/// TransitionPhase は ForEach の最中に呼ばれるため、その場でアーキタイプを
		/// 動かせず変更を予約する。予約したままにすると次の BeginFrame まで反映されず、
		/// フェーズが1段進むのに1フレームかかってしまう。
		///
		/// 反復が終わった直後にこれを呼べば同じフレームのうちに反映できる。
		/// 反復中に呼んではいけない(チャンクの並びが変わる)。
		/// </remarks>
		void ApplyChangeSignatures();

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

		// システム実体の寿命を預ける。
		// 生成と Init(タスク登録)は上位層が済ませてから渡すこと
		void HoldSystem(std::shared_ptr<ISystem> a_spSystem) { m_systemManager.Hold(std::move(a_spSystem)); }

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

		// カスタムタスク登録
		// システム内で何度もForEachなどを使うときに使用
		template<typename ...Read, typename... Write, typename Func>
		void RegisterCustomTask(ESystemType a_phase, ReadList<Read...>,WriteList<Write...>,Func a_func);

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

	protected:

		//==========================================================================================
		//
		// 派生への差し込み口
		//
		// 基盤はエンティティの入れ物と実行の仕組みだけを持ち、
		// 「生まれた直後に何を載せるか」「消える前に何を走らせるか」といった
		// ライフサイクルの決めごとは持たない。
		// その決めごとを持つ層(App::ECS::World)がここを override する。
		//
		//==========================================================================================

		// ※ タイプIDの取得(GetCompTypeID)が非constなので、フックも非constで持つ

		/// <summary>新しく作るエンティティのシグネチャへ、初期状態を載せる</summary>
		virtual void OnCreateEntitySignature(Signature& a_sig) { (void)a_sig; }

		/// <summary>構成が変わったエンティティを、初期化からやり直させる</summary>
		virtual void OnReenterInitSignature(Signature& a_sig) { (void)a_sig; }

		/// <summary>初期化のやり直しに入るかどうか(借りているものを返す判断に使う)</summary>
		virtual bool IsReenteringInit(const Signature& a_from, const Signature& a_to)
		{
			(void)a_from; (void)a_to; return false;
		}

		/// <summary>エンティティの増減・引っ越しがあった</summary>
		virtual void OnEntityStructureChanged() {}

		/// <summary>リフレッシュリストにたまったエンティティを一括で処理する</summary>
		virtual void RefreshEntities();

		//------------------------------------------------------------------------------------------
		// エンティティの即時削除
		//
		// 後始末を通さずに消すので、外からは使わせない。
		// 片付け終わったエンティティを実際に消す最後の一手として、
		// BeginFrame / Release からのみ呼ぶこと。削除の入口は AddReleaseEntity。
		//------------------------------------------------------------------------------------------

		// 削除予定エンティティを追加
		void AddRemoveEntity(const Entity& a_entity);

		// エンティティの削除
		void RemoveEntity(const Entity& a_entity);

	protected:

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
		virtual ~World();

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

				// 問い合わせ専用のタグ(App::ECS のフェーズタグなど)は「データ」ではなく
				// 絞り込み条件なので、実行順を決める依存(read/write)には含めない。
				//
				// 含めてしまうと、ActiveTask は先頭に ActiveTag を非constで足すので
				// 「全ての ActiveTask が ActiveTag の書き手」になる。一方 ActiveCustomTask は
				// ActiveTag を読み手として持つため、カスタムタスクが書いた成分を読む
				// ActiveTask が1つでも現れた瞬間に相互依存(循環)が成立して
				// トポロジカルソートが失敗する。タグは誰も書き換えないので外すのが正しい。
				//
				// どの型がタグなのかは IsQueryOnlyTag の特殊化で上位層が宣言する。
				// 問い合わせ用のシグネチャは DispatchTask が Components... から作り直すので、
				// ここで外してもタグによる絞り込みは効いたまま。
				if constexpr (!IsQueryOnlyTag_v<_CompType>)
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