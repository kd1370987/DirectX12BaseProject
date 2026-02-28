#include "World.h"

#include "../Internal/EntityLocation.h"

void World::Init()
{
	// エンティティマネージャー作成
	m_entityManager.Init();

	// アーキタイプチャンクマネージャー作成
	m_archetypeChunkManager.Init(&m_componentMetaRegistry);

	// システムマネージャー
	m_systemManager.Init();

	// 初期化済み
	m_isInit = true;
}
bool World::IsInit()
{
	return m_isInit;
}

void World::Release()
{
	
}

void World::ClaerMemory()
{
	m_entityManager.Init();
}

ECS::Entity World::CreateEntity(const ECS::Signature& a_sig)
{
	// エンティティIDの生成
	ECS::Entity _entity = m_entityManager.CreateEntity(a_sig);

	// エンティティをチャンクに割り当てる
	EntityLocation _loca = m_archetypeChunkManager.AllocateEntity(_entity, a_sig);

	// エンティティのロケーションを記録
	m_entityManager.SetEntityLocation(_entity,_loca);

	return _entity;
}

const std::vector<EntityLocation>& World::GetEntityList()
{
	return m_entityManager.GetAllEntityLocation();
}

const EntityLocation& World::GetLocation(const ECS::Entity& a_entity)
{
	return m_entityManager.GetLocation(a_entity);
}

UINT World::GetAliveEntityCount()
{
	return m_entityManager.GetAliveEntityCount();
}

const ECS::Entity& World::GetEntity(const EntityLocation& a_location)
{
	if (!a_location.pArchetypeChunk) return ECS::Limits::INVALID_ENTITY;

	return a_location.pArchetypeChunk->entityData[a_location.chunkIndex];
}

ECS::Signature World::GetSignature(const ECS::Entity& a_entity)
{
	return m_entityManager.GetSignature(a_entity);
}

void World::SpawnEntity()
{
}

void World::RemoveEntityStorage()
{
	// 消去予定エンティティがなければスキップ
	if (m_removeEntityStorage.size() == 0) return;

	// ストレージにあるのは消去
	for (auto& _entity : m_removeEntityStorage)
	{
		RemoveEntity(_entity);
	}

	// 空にする
	m_removeEntityStorage.clear();

	// メモリだけ確保
	m_removeEntityStorage.reserve(100);
}

void World::AddRemoveEntity(const ECS::Entity& a_entity)
{
	m_removeEntityStorage.push_back(a_entity);
}

void World::RemoveEntity(const ECS::Entity& a_entity)
{
	// ロケーション取得
	const auto& _loca = m_entityManager.GetLocation(a_entity);
	if (!_loca.pArchetypeChunk)return;

	// アーキタイプから削除して、移動したエンティティの情報をもらう
	auto [_entity,_idx] = m_archetypeChunkManager.RemoveEntity(_loca);

	// エンティティマネージャーからも消去
	m_entityManager.DestroyEntity(a_entity);

	// 移動したエンティティのロケーションを変更
	auto& _swapLoca = m_entityManager.RefEntityLocation(_entity);
	_swapLoca.chunkIndex = _idx;
}

ECS::ComponentTypeID World::GetCompTypeID(const std::type_index& a_index)
{
	return m_componentMetaRegistry.GetTypeID(a_index);
}

uint8_t* World::NRefData(const ECS::Entity& a_entity, const std::type_index& a_index)
{
	const EntityLocation& _loca = m_entityManager.GetLocation(a_entity);
	ECS::ComponentTypeID _typeID = m_componentMetaRegistry.GetTypeID(a_index);
	return m_archetypeChunkManager.RefComponent(_loca, _typeID);
}

uint8_t* World::NRefData(const ECS::Entity& a_entity, const ECS::ComponentTypeID& a_typeID)
{
	const EntityLocation& _loca = m_entityManager.GetLocation(a_entity);
	return m_archetypeChunkManager.RefComponent(_loca, a_typeID);
}


const ComponentMeta& World::GetComponentMetaData(const ECS::ComponentTypeID& a_typeID)
{
	return m_componentMetaRegistry.GetMetaData(a_typeID);
}

void World::RunSystem(SystemType a_type, float a_dt)
{
	m_systemManager.RunSystem(*this, a_type, a_dt);
}

// コンストラクタ・デストラクタ
World::World()
{
}
World::~World()
{
}
