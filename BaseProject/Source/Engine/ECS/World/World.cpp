#include "World.h"

#include "../Internal/EntityLocation.h"

void World::Init()
{
	// エンティティマネージャー作成
	m_entityManager.Init();

	// アーキタイプチャンクマネージャー作成
	m_archetypeChunkManager.Init(&m_componentMetaRegistry);
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
