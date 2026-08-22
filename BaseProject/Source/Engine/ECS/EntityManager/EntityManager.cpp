#include "EntityManager.h"

namespace Engine::ECS
{

	// コンストラクタ・デストラクタ
	EntityManager::EntityManager()
	{}
	EntityManager::~EntityManager()
	{}

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

		// 無効なIDが混じっても落とさない。
		// 世代を見る前に添え字で引くので、確かめるのはこちらが先
		if (a_entity == ECS::Limits::INVALID_ENTITY) return;
		if (_idx >= m_entityGeneVec.size()) return;

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

	//======================================================================================
	// 住所の取得
	//--------------------------------------------------------------------------------------
	// GetSignature と同じ理由で、無効なエンティティが来ても落ちないようにしてある。
	// 空の住所(チャンク無し)を返すと、この先の RefComponent が nullptr を返すので、
	// 「持っていない」として扱われる。
	//======================================================================================
	const EntityLocation& EntityManager::GetLocation(const ECS::Entity& a_entity)
	{
		static const EntityLocation _emptyLoca = {};

		if (a_entity == ECS::Limits::INVALID_ENTITY) return _emptyLoca;

		uint32_t _idx = GetIndex(a_entity);

		// 別ワールドのIDなどで添え字が範囲外になることがある
		if (_idx >= m_entityLocationVec.size()) return _emptyLoca;

		return m_entityLocationVec[_idx];
	}

	bool EntityManager::IsAlive(const ECS::Entity& a_entity)
	{
		if (a_entity == ECS::Limits::INVALID_ENTITY) return false;

		const uint32_t _idx = GetIndex(a_entity);
		const uint32_t _gen = GetGeneration(a_entity);

		// 別ワールドのIDなどで添え字が範囲外になることがある
		if (_idx >= m_entityLocationVec.size()) return false;

		// 削除のたびに世代が進むので、使い回された添え字はここで弾ける
		if (m_entityGeneVec[_idx] != _gen) return false;

		// 生成済みなら必ずチャンクに載っている。
		// 一度も使われていない添え字(世代0のまま)はここで落ちる
		return m_entityLocationVec[_idx].pArchetypeChunk != nullptr;
	}

	EntityLocation& EntityManager::RefEntityLocation(const ECS::Entity& a_entity)
	{
		uint32_t _idx = GetIndex(a_entity);

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

	//======================================================================================
	// シグネチャの取得
	//--------------------------------------------------------------------------------------
	// 無効なエンティティが来ることを前提にしてある。
	//
	// HasComponent はここを通るので、「持っているかどうか」を確かめる側は
	// 相手が居るかどうかを気にせず呼べる必要がある。
	// 見つからなかった参照は INVALID_ENTITY(=UINT64_MAX)で返ってくるため、
	// そのまま添え字にすると範囲外を読んで落ちる。
	// (例 : 追従先のGUIDがそのシーンに居ないカメラ → HasComponent で弾くつもりが
	//       HasComponent 自体で落ちていた)
	//
	// 空のシグネチャを返せば「何も持っていない」扱いになり、
	// 呼び出し側の HasComponent 判定がそのまま効く。
	//======================================================================================
	const ECS::Signature& EntityManager::GetSignature(const ECS::Entity& a_entity)
	{
		static const ECS::Signature _emptySig = {};

		if (a_entity == ECS::Limits::INVALID_ENTITY) return _emptySig;

		uint32_t _idx = GetIndex(a_entity);

		// 別ワールドのIDなどで添え字が範囲外になることがある
		if (_idx >= m_signatureVec.size()) return _emptySig;

		return m_signatureVec[_idx];
	}

	void EntityManager::SetSignature(const Entity& a_entity, const Signature& a_sig)
	{
		uint32_t _index = GetIndex(a_entity);
		m_signatureVec[_index] = a_sig;
	}

	uint32_t EntityManager::GetGeneration(const ECS::Entity& a_entity)
	{
		return uint32_t(a_entity >> 32);;
	}

	uint32_t EntityManager::GetIndex(const ECS::Entity& a_entity)
	{
		return uint32_t(a_entity & 0xFFFFFFFF);
	}

}