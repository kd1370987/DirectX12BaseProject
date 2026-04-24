#pragma once

#include "../EntityManager/EntityManager.h"
#include "../SystemManager/SystemManager.h"
#include "../ArchetypeChunkManager/ArchetypeChunkManager.h"
#include "../ArchetypeChunk/ArchetypeChunk.h"

#include "../ComponentMetaRegistry/ComponentMetaRegistry.h"

#include "../Internal/SystemComon.h"

class ActiveTag;
class StartTag;
class AwekeTag;
class PostDeserializeTag;

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
		std::unordered_map<ComponentTypeID, uint8_t*> dataMap = {};
	};


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
		void ClaerMemory();	// 任意のリセットしたいタイミング

		// フレームの初めに呼び出す関数
		// シングルフレームで実行したい、生成や破棄、引っ越しを行う
		void BegineFrame();

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
		/// ロケーションからエンティティを取得
		/// </summary>
		const Entity& GetEntity(const EntityLocation& a_location);

		/// <summary>
		/// エンティティのシグネチャを取得
		/// </summary>
		Signature GetSignature(const Entity& a_entity);

		//------------------------------------------------------------------------------------------
		// エンティティの生成
		//------------------------------------------------------------------------------------------

		void AddEntity(const Signature& a_sig);			// コマンド発行
		Entity CreateEntity(const Signature& a_sig);	// 実体の作成
		void CreateAllEntity();							// 一括作成

		//------------------------------------------------------------------------------------------
		// エンティティの削除
		//------------------------------------------------------------------------------------------

		// フレームの初めにエンティティを削除する
		void RemoveEntityStorage();

		// 削除予定エンティティを追加
		void AddRemoveEntity(const Entity& a_entity);

		// エンティティの削除
		void RemoveEntity(const Entity& a_entity);

		//------------------------------------------------------------------------------------------
		// エンティティの操作
		//------------------------------------------------------------------------------------------
		// エンティティに対してコンポーネントを追加
		void AddComponent(ComponentTypeID a_typeID,Entity a_entity);
		void SubmitComponent(ComponentTypeID a_typeID,Entity a_entity);
		void AddChangeSigCommand(ChangeEntityCmd a_cmd);
		void ChangeSigneture(ChangeEntityCmd a_cmd);

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

		/// <summary>
		/// ネイティブなバイトデータへのポインタを取得
		/// </summary>
		/// <param name="a_entity">エンティティID</param>
		/// <param name="a_index">コンポーネント型</param>
		/// <returns></returns>
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

		/// <summary>
		/// コンポーネントメタデータの取得
		/// </summary>
		const ComponentMeta& GetComponentMetaData(const ComponentTypeID& a_typeID);
		const std::unordered_map<ComponentTypeID, ComponentMeta>& GetAllComponentMetaData() const;
		const ComponentFunc& GetCompFunc(const ComponentTypeID& a_typeID)const;

		//==========================================================================================
		// 
		// システム関連
		// 
		//==========================================================================================

		/// <summary>
		/// システムの登録
		/// </summary>
		/// <typeparam name="System">登録したいクラス</typeparam>
		template<typename System>
		void RegisterSystem();

		/// <summary>
		/// システムの実行
		/// </summary>
		/// <param name="a_type">実行してほしいシステムタイプ</param>
		/// <param name="a_dt">デルタタイム</param>
		void RunSystem(ESystemType a_type, float a_dt);

		/// <summary>
		/// 指定したコンポーネント群を持つすべてのチャンクに対して、指定された関数を実行します
		/// </summary>
		/// <typeparam name="...Components">必要なコンポーネント</typeparam>
		/// <typeparam name="Func">引数を入れる関数</typeparam>
		/// <param name="a_func">引数を入れる関数</param>
		template<typename... Components, typename Func>
		void ForEach(Func a_func);

		/// <summary>
		/// 除外指定もできる検索
		/// </summary>
		/// <typeparam name="...Components">必要なコンポーネント</typeparam>
		/// <typeparam name="...Excludes">除外するコンポーネント</typeparam>
		template<typename... Components, typename... Excludes, typename Func>
		void ForEachEx(Func a_func, Exclude<Excludes...>);

		// システムフェーズごとのForEach処理
		// モデルやテクスチャなどのシリアライズ後の依存関係解決する処理
		template<typename... Components, typename... Excludes, typename Func>
		void ForEachPostDeserialize(Func a_func, Exclude<Excludes...> a_ex = {});

		// エンティティ以外での依存解決が終わった後に回す処理
		template<typename... Components, typename... Excludes, typename Func>
		void ForEachAweke(Func a_func, Exclude<Excludes...> a_ex = {});

		// アップデートの直前に回す処理
		template<typename... Components, typename... Excludes, typename Func>
		void ForEachStart(Func a_func, Exclude<Excludes...> a_ex = {});

		// 更新描画
		template<typename... Components, typename... Excludes, typename Func>
		void ForEachUpdate(Func a_func, Exclude<Excludes...> a_ex = {});

		// システムのフェーズ遷移
		void TransitionToAwake();		// ポストデシリアライズからアウェイク処理へ
		void TransitionToStart();		// アウェイクからスタート処理へ
		void TransitionToUpdate();		// スタートからからアップデートへ処理へ

		template<typename Beffor,typename Affter>
		void TransitionPhase();

	private:

		// マネージャー軍
		EntityManager m_entityManager;
		SystemManager m_systemManager;
		ArchetypeChunkManager m_archetypeChunkManager;

		// コンポーネントメタ情報管理
		ComponentMetaRegistry m_componentMetaRegistry;

		// 初期化済み
		bool m_isInit = false;

		// 生成予定エンティティリスト
		std::vector<Signature> m_addEntityVec = {};

		// 削除予定エンティティ
		std::vector<Entity> m_removeEntityVec = {};

		// 移動予定エンティティ
		std::vector<ChangeEntityCmd> m_changeEntityVec = {};

	public:
		// コンストラクタデストラクタ
		World();
		~World();

		// コピー禁止
		World(const World&) = default;
		World& operator=(const World&) = default;

		// ムーブ禁止
		World(World&&) = default;
		World& operator = (World&&) = default;
	};

	template<typename Comp>
	inline ComponentTypeID World::RegisterComponent(const std::string& a_name)
	{
		auto _id = m_componentMetaRegistry.RegisterType<Comp>(a_name);
		return _id;
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
		return reinterpret_cast<Comp*>(
			m_archetypeChunkManager.RefComponentArray(a_chunk, m_componentMetaRegistry.GetTypeID<Comp>())
			);
	}

	template<typename System>
	inline void World::RegisterSystem()
	{
		m_systemManager.Register<System>();
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
	template<typename ...Components, typename ...Excludes, typename Func>
	inline void World::ForEachPostDeserialize(Func a_func, Exclude<Excludes...> a_ex)
	{
		ForEachEx<PostDeserializeTag, Components...>(a_func, a_ex);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline void World::ForEachAweke(Func a_func, Exclude<Excludes...> a_ex)
	{
		ForEachEx<AwekeTag, Components...>(a_func, a_ex);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline void World::ForEachStart(Func a_func, Exclude<Excludes...> a_ex)
	{
		ForEachEx<StartTag, Components...>(a_func, a_ex);
	}
	template<typename ...Components, typename ...Excludes, typename Func>
	inline void World::ForEachUpdate(Func a_func, Exclude<Excludes...> a_ex)
	{
		ForEachEx<ActiveTag, Components...>(a_func, a_ex);
	}
	template<typename Beffor, typename Affter>
	inline void World::TransitionPhase()
	{
		ForEach<Beffor>(
			[]
			(
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				Beffor* a_compArray
			)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					Beffor& _comp = a_compArray[_i];
				}
			}
		);
	}
}