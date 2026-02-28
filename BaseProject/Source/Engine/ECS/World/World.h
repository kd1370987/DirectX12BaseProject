#pragma once

#include "../EntityManager/EntityManager.h"
#include "../SystemManager/SystemManager.h"
#include "../ArchetypeChunkManager/ArchetypeChunkManager.h"
#include "../ArchetypeChunk/ArchetypeChunk.h"

#include "../ComponentMetaRegistry/ComponentMetaRegistry.h"

#include "../Internal/SystemComon.h"

template<typename... Excludes>
struct Exclude {};

class World
{
public:

	//==========================================================================================
	// 
	// ワールドに対しての操作関連
	// 
	//==========================================================================================

	/// <summary>
	/// 初期化
	/// </summary>
	void Init();

	/// <summary>
	/// 初期化済みかどうか
	/// </summary>
	bool IsInit();

	/// <summary>
	/// 解放
	/// </summary>
	void Release();

	/// <summary>
	/// すべてのデータのリセット
	/// </summary>
	void ClaerMemory();

	//==========================================================================================
	// 
	// エンティティ関連
	// 
	//==========================================================================================

	/// <summary>
	/// エンティティの生成
	/// </summary>
	/// <param name="a_sig">アーキタイプを指定</param>
	ECS::Entity CreateEntity(const ECS::Signature& a_sig);

	/// <summary>
	/// 生存中のエンティティリストを返す
	/// </summary>
	const std::vector<EntityLocation>& GetEntityList();

	/// <summary>
	/// エンティティのロケーションを取得
	/// </summary>
	const EntityLocation& GetLocation(const ECS::Entity& a_entity);

	/// <summary>
	/// 生存エンティティ数を返す
	/// </summary>
	UINT GetAliveEntityCount();

	/// <summary>
	/// ロケーションからエンティティを取得
	/// </summary>
	const ECS::Entity& GetEntity(const EntityLocation& a_location);

	/// <summary>
	/// エンティティのシグネチャを取得
	/// </summary>
	ECS::Signature GetSignature(const ECS::Entity& a_entity);

	/// <summary>
	/// フレーム初めにエンティティを生成する
	/// </summary>
	void SpawnEntity();

	//------------------------------------------------------------------------------------------
	// エンティティの削除
	//------------------------------------------------------------------------------------------

	// フレームの初めにエンティティを削除する
	void RemoveEntityStorage();
	
	// 削除予定エンティティを追加
	void AddRemoveEntity(const ECS::Entity& a_entity);

	// エンティティの削除
	void RemoveEntity(const ECS::Entity& a_entity);

	//==========================================================================================
	// 
	// コンポーネント関連
	// 
	//==========================================================================================

	/// <summary>
	/// コンポーネントの型情報を登録
	/// </summary>
	/// <typeparam name="Comp">コンポーネントの型</typeparam>
	/// <param name="a_name">保存時の名前</param>
	template<typename Comp>
	ECS::ComponentTypeID RegisterComponentType(const std::string& a_name);

	/// <summary>
	/// 型情報からIDを取得
	/// </summary>
	/// <param name="a_index"></param>
	/// <returns></returns>
	ECS::ComponentTypeID GetCompTypeID(const std::type_index& a_index);

	/// <summary>
	/// ネイティブなバイトデータへのポインタを取得
	/// </summary>
	/// <param name="a_entity">エンティティID</param>
	/// <param name="a_index">コンポーネント型</param>
	/// <returns></returns>
	uint8_t* NRefData(const ECS::Entity& a_entity, const std::type_index& a_index);
	uint8_t* NRefData(const ECS::Entity& a_entity, const ECS::ComponentTypeID& a_typeID);

	/// <summary>
	/// コンポーネントを型として参照取得
	/// </summary>
	/// <typeparam name="Comp">型情報</typeparam>
	/// <param name="a_entity">エンティティID</param>
	template<typename Comp>
	Comp* RefData(const ECS::Entity& a_entity);

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
	const ComponentMeta& GetComponentMetaData(const ECS::ComponentTypeID& a_typeID);

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
	void RunSystem(SystemType a_type, float a_dt);

	/// <summary>
	/// 指定したコンポーネント群を持つすべてのチャンクに対して、指定された関数を実行します
	/// </summary>
	/// <typeparam name="...Components">必要なコンポーネント</typeparam>
	/// <typeparam name="Func">引数を入れる関数</typeparam>
	/// <param name="a_func">引数を入れる関数</param>
	template<typename... Components,typename Func>
	void ForEach(Func a_func);

	/// <summary>
	/// 除外指定もできる検索
	/// </summary>
	/// <typeparam name="...Components">必要なコンポーネント</typeparam>
	/// <typeparam name="...Excludes">除外するコンポーネント</typeparam>
	template<typename... Components,typename... Excludes, typename Func>
	void ForEachEx(Func a_func, Exclude<Excludes...>);

private:

	// マネージャー軍
	EntityManager m_entityManager;
	SystemManager m_systemManager;
	ArchetypeChunkManager m_archetypeChunkManager;

	// コンポーネントメタ情報管理
	ComponentMetaRegistry m_componentMetaRegistry;

	// 初期化済み
	bool m_isInit = false;

	// 生成予定エンティティ
	std::vector<ECS::Entity> m_spawnEntityStorage;

	// 削除予定エンティティ
	std::vector<ECS::Entity> m_removeEntityStorage;

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
inline ECS::ComponentTypeID World::RegisterComponentType(const std::string& a_name)
{
	return m_componentMetaRegistry.RegisterType<Comp>(a_name);
}

template<typename Comp>
inline Comp* World::RefData(const ECS::Entity& a_entity)
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
	ECS::Signature _sig;
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
	ECS::Signature _sig;
	(_sig.set(m_componentMetaRegistry.GetTypeID<Components>()), ...);
	ECS::Signature _excludeSig;
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
