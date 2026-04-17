#include "World.h"

#include "../Internal/EntityLocation.h"

namespace Engine::ECS
{

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

	void World::BegineFrame()
	{
		// エンティティの一括作成
		CreateAllEntity();

		// エンティティの引っ越し
		for (auto _chanCmd : m_changeEntityVec)
		{
			ChangeSigneture(_chanCmd);
		}
		m_changeEntityVec.clear();

		// エンティティの一括削除
		RemoveEntityStorage();
	}

	void World::AddEntity(const Signature& a_sig)
	{
		m_addEntityVec.push_back(a_sig);
	}

	ECS::Entity World::CreateEntity(const ECS::Signature& a_sig)
	{
		// エンティティIDの生成
		ECS::Entity _entity = m_entityManager.CreateEntity(a_sig);

		// エンティティをチャンクに割り当てる
		EntityLocation _loca = m_archetypeChunkManager.AllocateEntity(_entity, a_sig);

		// エンティティのロケーションを記録
		m_entityManager.SetEntityLocation(_entity, _loca);

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

	void World::CreateAllEntity()
	{
		for (auto& _sig : m_addEntityVec)
		{
			CreateEntity(_sig);
		}
		m_addEntityVec.clear();
	}

	void World::RemoveEntityStorage()
	{
		// 消去予定エンティティがなければスキップ
		if (m_removeEntityVec.size() == 0) return;

		// ストレージにあるのは消去
		for (auto& _entity : m_removeEntityVec)
		{
			RemoveEntity(_entity);
		}

		// 空にする
		m_removeEntityVec.clear();

		// メモリだけ確保
		m_removeEntityVec.reserve(100);
	}

	void World::AddRemoveEntity(const ECS::Entity& a_entity)
	{
		m_removeEntityVec.push_back(a_entity);
	}

	void World::RemoveEntity(const ECS::Entity& a_entity)
	{
		// ロケーション取得
		const auto& _loca = m_entityManager.GetLocation(a_entity);
		if (!_loca.pArchetypeChunk)return;

		// アーキタイプから削除して、移動したエンティティの情報をもらう
		auto [_entity, _idx] = m_archetypeChunkManager.RemoveEntity(_loca);

		// エンティティマネージャーからも消去
		m_entityManager.DestroyEntity(a_entity);

		// 移動したエンティティのロケーションを変更
		auto& _swapLoca = m_entityManager.RefEntityLocation(_entity);
		_swapLoca.chunkIndex = _idx;
	}

	void World::AddComponent(ComponentTypeID a_typeID, Entity a_entity)
	{
		// エンティティのシグネチャを変更
		Signature _oldSig = m_entityManager.GetSignature(a_entity);
		// 新たにシグネチャを作成
		if (_oldSig.test(a_typeID)) return;		// すでに持っていたらリターン
		_oldSig.set(a_typeID);

		// 命令の発行
		ChangeEntityCmd	_cmd = {};
		_cmd.entity = a_entity;
		_cmd.toSig = _oldSig;
		m_changeEntityVec.push_back(_cmd);
	}

	void World::ChangeSigneture(ChangeEntityCmd a_cmd)
	{
		// エンティティシグネチャの取得
		const Signature& _oldSig = m_entityManager.GetSignature(a_cmd.entity);
		const EntityLocation& _oldLoca = m_entityManager.GetLocation(a_cmd.entity);
		// 古いエンティティのデータを一時的に記憶
		std::queue<uint8_t*> _oldData = {};
		for (ComponentTypeID _compID = 0; _compID < _oldSig.size(); ++_compID)
		{
			if (!_oldSig.test(_compID)) continue;
			_oldData.push(NRefData(a_cmd.entity,_compID));
		}

		// エンティティの削除
		{
			// アーキタイプから削除して、移動したエンティティの情報をもらう
			auto [_entity, _idx] = m_archetypeChunkManager.RemoveEntity(_oldLoca);

			// 移動したエンティティのロケーションを変更
			auto& _swapLoca = m_entityManager.RefEntityLocation(_entity);
			_swapLoca.chunkIndex = _idx;
		}

		// 新しい場所にエンティティを割り当てる
		EntityLocation _loca = m_archetypeChunkManager.AllocateEntity(a_cmd.entity,a_cmd.toSig);

		// 新しいシグネチャのデータを初期化する
		for (ComponentTypeID _compID = 0; _compID < a_cmd.toSig.size(); ++_compID)
		{
			// 前のシグネチャと一致していたらそのデータをコピー
			if (_oldSig.test(_compID))
			{
				uint8_t* _pData = NRefData(a_cmd.entity,_compID);
				memcpy(_pData,_oldData.front(),GetComponentMetaData(_compID).compSize);
				_oldData.pop();
			}

			// 指定されたデータがあればこっちで上書き
			auto _it = a_cmd.dataMap.find(_compID);
			if (_it != a_cmd.dataMap.end())
			{
				uint8_t* _pData = NRefData(a_cmd.entity, _compID);
				memcpy(_pData, _it->second, GetComponentMetaData(_compID).compSize);
			}
		}

		// エンティティのロケーションを記録
		m_entityManager.SetEntityLocation(a_cmd.entity, _loca);
		m_entityManager.SetSignature(a_cmd.entity,a_cmd.toSig);
	}

	void World::MoveEntityToArchetype(Entity a_entity, ArchetypeChunk* a_pChunk, Signature a_sig)
	{
		// 古いチャンクからデータを取得し新たなアーキタイプに移動
		EntityLocation _oldLoc = m_entityManager.GetLocation(a_entity);
		Signature _oldSig = m_entityManager.GetSignature(a_entity);

		// 新しいチャンクにコピー
		
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

	const std::unordered_map<ComponentTypeID, ComponentMeta>& World::GetAllComponentMetaData() const
	{
		return m_componentMetaRegistry.GetAllMetaData();
	}

	void World::RunSystem(ESystemType a_type, float a_dt)
	{
		m_systemManager.RunSystem(*this, a_type, a_dt);
	}

	World::World()
	{}

	World::~World()
	{}

}