#pragma once

class EntityManager;
class SystemManager;
class ArchetypeChunkManager;

class ComponentMetaRegistry;

class World
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Init();

	/// <summary>
	/// 解放
	/// </summary>
	void Release();

	/// <summary>
	/// すべてのデータのリセット
	/// </summary>
	void ClaerMemory();

	/// <summary>
	/// エンティティの生成
	/// </summary>
	/// <param name="a_sig">アーキタイプを指定</param>
	ECS::Entity CreateEntity(const ECS::Signature& a_sig);

	/// <summary>
	/// 型情報からIDを取得
	/// </summary>
	/// <param name="a_index"></param>
	/// <returns></returns>
	ECS::ComponentTypeID GetCompTypeID(const std::type_index& a_index);

	uint8_t* RefData(const ECS::Entity& a_entity, const std::type_index& a_index);

private:

	// マネージャー軍
	std::unique_ptr<EntityManager> m_upEntityManager = nullptr;
	std::unique_ptr<SystemManager> m_upSystemManager = nullptr;
	std::unique_ptr<ArchetypeChunkManager> m_upArchetypeChunkManager = nullptr;

	std::shared_ptr<ComponentMetaRegistry> m_spComponentMetaRegistry = nullptr;


// シングルトン
private:

	World();
	~World();

	// コピー禁止
	World(const World&) = default;
	World& operator=(const World&) = default;

	// ムーブ禁止
	World(World&&) = default;
	World& operator = (World&&) = default;

public:

	static World& Instance()
	{
		static World _instance;
		return _instance;
	}
};