#include "World.h"

#include "../EntityManager/EntityManager.h"
#include "../SystemManager/SystemManager.h"
#include "../ArchetypeChunkManager/ArchetypeChunkManager.h"

#include "../ComponentMetaRegistry/ComponentMetaRegistry.h"

#include "../Internal/EntityLocation.h"

#include "../../../Application/Components/TransformComponent.h"

void World::Init()
{
	// コンポーネントメタレジストリの作成
	m_spComponentMetaRegistry = std::make_shared<ComponentMetaRegistry>();
	m_spComponentMetaRegistry->RegisterType<TransformComponent>("TrasformComponent");

	// エンティティマネージャー作成
	m_upEntityManager = std::make_unique<EntityManager>();
	m_upEntityManager->Init();

	// システムマネージャー作成
	m_upSystemManager = std::make_unique<SystemManager>();

	// アーキタイプチャンクマネージャー作成
	m_upArchetypeChunkManager = std::make_unique<ArchetypeChunkManager>(m_spComponentMetaRegistry.get());
}
void World::Release()
{
}

void World::ClaerMemory()
{
	m_upEntityManager->Init();
}

ECS::Entity World::CreateEntity(const ECS::Signature& a_sig)
{
	// エンティティIDの生成
	ECS::Entity _entity = m_upEntityManager->CreateEntity(a_sig);

	// エンティティをチャンクに割り当てる
	EntityLocation _loca = m_upArchetypeChunkManager->AllocateEntity(_entity, a_sig);

	// エンティティのロケーションを記録
	m_upEntityManager->SetEntityLocation(_entity,_loca);

	return _entity;
}

ECS::ComponentTypeID World::GetCompTypeID(const std::type_index& a_index)
{
	return m_spComponentMetaRegistry->GetTypeID(a_index);
}

uint8_t* World::RefData(const ECS::Entity& a_entity, const std::type_index& a_index)
{
	const EntityLocation& _loca = m_upEntityManager->GetLocation(a_entity);
	ECS::ComponentTypeID _typeID = m_spComponentMetaRegistry->GetTypeID(a_index);
	return m_upArchetypeChunkManager->RefComponent(_loca,_typeID);
}

// コンストラクタ・デストラクタ
World::World()
{
}
World::~World()
{
}
