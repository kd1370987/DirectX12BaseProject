#include "World.h"

#include "../Internal/EntityLocation.h"

// エンティティに初めからつけるためこの二つはインクルード
#include "../../../Application/Components/Persistence/GUIDComponent.h"
#include "../../../Application/Components/Persistence/NameComponent.h"

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
		// システムのソート
		m_systemManager.Sort();

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
		Signature _sig = a_sig;
		_sig.set(GetCompTypeID<PostDeserializeTag>());		// 初めて通るシステムフェーズ
		_sig.set(GetCompTypeID<GUIDComponent>());			// オブジェクトとして追加するときは必ず付与
		_sig.set(GetCompTypeID<NameComponent>());

		if (_sig.test(GetCompTypeID<ActiveTag>()))
		{
			_sig.reset(GetCompTypeID<ActiveTag>());
		}

		ECS::Entity _entity = m_entityManager.CreateEntity(_sig);

		// エンティティをチャンクに割り当てる
		EntityLocation _loca = m_archetypeChunkManager.AllocateEntity(_entity, _sig);

		// エンティティのロケーションを記録
		m_entityManager.SetEntityLocation(_entity, _loca);

		// シグネチャごとにコンストラクタを回す
		for (ComponentTypeID _i = 0; _i < _sig.size(); ++_i)
		{
			if (!_sig.test(_i)) continue;

			uint8_t* _data = NRefData(_entity, _i);
			
		}

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

	Entity World::GetEntity(const Engine::GUID& a_guid)
	{
		Entity _res = Limits::INVALID_ENTITY;

		ForEach<GUIDComponent>(
			[&a_guid,&_res](
				ArchetypeChunk* a_chunk,
				uint32_t a_count,
				GUIDComponent* a_guidArray
			)
			{ 

				if (_res != Limits::INVALID_ENTITY) return;

				for(size_t _i= 0; _i < a_count; ++_i)
				{
					if (_res != Limits::INVALID_ENTITY) continue;

					GUIDComponent& _comp = a_guidArray[_i];
					if (_comp.guid == a_guid)
					{
						_res = a_chunk->entityData[_i];
					}
				}
			}
		);

		return _res;
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

	void World::SubmitComponent(ComponentTypeID a_typeID, Entity a_entity)
	{
		// エンティティのシグネチャを変更
		Signature _oldSig = m_entityManager.GetSignature(a_entity);

		// 新たにシグネチャを作成
		if (!_oldSig.test(a_typeID)) return;	// 持っていなければコマンドを発行しない
		_oldSig.reset(a_typeID);

		AddChangeSigCommand({
			.entity = a_entity,
			.toSig = _oldSig,
		});
	}

	void World::AddChangeSigCommand(ChangeEntityCmd a_cmd)
	{
		m_changeEntityVec.push_back(a_cmd);
	}

	void World::ChangeSigneture(ChangeEntityCmd a_cmd)
	{
		// エンティティシグネチャの取得
		const Signature& _oldSig = m_entityManager.GetSignature(a_cmd.entity);
		const EntityLocation& _oldLoca = m_entityManager.GetLocation(a_cmd.entity);
		
		// 古いエンティティのデータを値として退避する
		std::unordered_map<ComponentTypeID, std::vector<uint8_t>> _oldData = {};

		for (ComponentTypeID _compID = 0; _compID < _oldSig.size(); ++_compID)
		{
			if (!_oldSig.test(_compID)) continue;

			size_t _size = GetComponentMetaData(_compID).compSize;

			std::vector<uint8_t> _buffer(_size);
			memcpy(_buffer.data(),NRefData(a_cmd.entity,_compID),_size);

			_oldData[_compID] = _buffer;
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

		// エンティティのロケーションを記録
		m_entityManager.SetEntityLocation(a_cmd.entity, _loca);
		m_entityManager.SetSignature(a_cmd.entity, a_cmd.toSig);

		// 新しいシグネチャのデータを初期化する
		for (ComponentTypeID _compID = 0; _compID < a_cmd.toSig.size(); ++_compID)
		{
			// 前のシグネチャと一致していたらそのデータをコピー
			if (_oldSig.test(_compID))
			{
				uint8_t* _pData = NRefData(a_cmd.entity,_compID);
				if (_oldData[_compID].data())
				{
					memcpy(_pData, _oldData[_compID].data(), GetComponentMetaData(_compID).compSize);
				}
			}

			// 指定されたデータがあればこっちで上書き
			auto _it = a_cmd.dataMap.find(_compID);
			if (_it != a_cmd.dataMap.end())
			{
				uint8_t* _pData = NRefData(a_cmd.entity, _compID);
				memcpy(_pData, _it->second, GetComponentMetaData(_compID).compSize);
			}
		}
	}

	ECS::ComponentTypeID World::GetCompTypeID(const std::type_index& a_index)
	{
		return m_componentMetaRegistry.GetTypeID(a_index);
	}

	ComponentTypeID World::GetCompTypeID(const std::string& a_name)
	{
		return m_componentMetaRegistry.GetTypeID(a_name);
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

	const ComponentFunc& World::GetCompFunc(const ComponentTypeID& a_typeID) const
	{
		return m_componentMetaRegistry.GetFunc(a_typeID);
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