#include "EntityManager.h"

// コンストラクタ・デストラクタ
EntityManager::EntityManager()
{
}
EntityManager::~EntityManager()
{
}

// 初期化
void EntityManager::Init()
{
	for (ECS::EntityIndex _entityIdx = 0; _entityIdx < ECS::Limits::MAX_ENTITIES; ++_entityIdx)
	{
		m_availbleEntitiyQueue.push(_entityIdx);
	}

	m_entityLocationVec.resize(ECS::Limits::MAX_ENTITIES);
	m_entityGeneVec.resize(ECS::Limits::MAX_ENTITIES);
	m_signatureVec.resize(ECS::Limits::MAX_ENTITIES);

	m_aliveCount = 0;
}

// エンティティの作成
ECS::Entity EntityManager::CreateEntity(const ECS::Signature& a_sig)
{
	if (m_availbleEntitiyQueue.empty())
	{
		assert(0 && "エンティティの生成上限に達しました");
		return ECS::Limits::INVALID_ENTITY;
	}

	// インデックス取得
	ECS::EntityIndex _idx = m_availbleEntitiyQueue.front();
	m_availbleEntitiyQueue.pop();

	// 世代の取得
	ECS::Generation _gen = m_entityGeneVec[_idx];
	
	// エンティティIDの生成
	ECS::Entity _entity = (uint64_t(_gen) << 32) | uint64_t(_idx);

	// シグネチャの記憶
	m_signatureVec[_idx] = a_sig;


	// 生存数インクリメント
	m_aliveCount++;

	return _entity;
}



// エンティティンの削除
void EntityManager::DestroyEntity(const ECS::Entity& a_entity)
{
	// 添え字の抽出
	uint32_t _idx = uint32_t(a_entity & 0xFFFFFFFF);
	uint32_t _gen = uint32_t(a_entity >> 32);

	if (m_entityGeneVec[_idx] != _gen)
	{
		return;
	}

	// エンティティ情報のリセット
	m_signatureVec[_idx].reset();
	m_entityLocationVec[_idx] = {};
	m_entityGeneVec[_idx]++;

	// 未使用キューに戻す
	m_availbleEntitiyQueue.push(_idx);

	// 生存数のデクリメント
	m_aliveCount--;
}

void EntityManager::SetEntityLocation(const ECS::Entity& a_entity, const EntityLocation& a_loca)
{
	// 添え字の抽出
	uint32_t _idx = uint32_t(a_entity & 0xFFFFFFFF);
	uint32_t _gen = uint32_t(a_entity >> 32);

	m_entityLocationVec[_idx] = a_loca;
}

const EntityLocation& EntityManager::GetLocation(const ECS::Entity& a_entity)
{
	uint32_t _idx = uint32_t(a_entity & 0xFFFFFFFF);
	uint32_t _gen = uint32_t(a_entity >> 32);

	return m_entityLocationVec[_idx];
}

const std::vector<EntityLocation>& EntityManager::GetAllEntityLocation()
{
	return m_entityLocationVec;
}

UINT EntityManager::GetAliveEntityCount()
{
	return m_aliveCount;
}

const ECS::Signature& EntityManager::GetSignature(const ECS::Entity& a_entity)
{
	uint32_t _idx = GetIndex(a_entity);
	return m_signatureVec[_idx];
}

uint32_t EntityManager::GetGeneration(const ECS::Entity& a_entity)
{
	return uint32_t(a_entity >> 32);;
}

uint32_t EntityManager::GetIndex(const ECS::Entity& a_entity)
{
	return uint32_t(a_entity & 0xFFFFFFFF);
}
