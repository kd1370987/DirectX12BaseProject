#include "World.h"

#include "../Internal/EntityLocation.h"

// エンティティに初めからつけるためこの二つはインクルード
#include "../../../Application/Components/Persistence/GUIDComponent.h"		// GUID
#include "../../../Application/Components/Persistence/NameComponent.h"		// 名前

// シングルトンリソース
#include "../../../Application/InstanceResource/HierarchyResource.h"		// 階層保持


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
		TransitionPhase<ActiveTag, ReleaseTag>();

		// エンティティの引っ越し
		for (auto& _chanCmd : m_changeEntityVec)
		{
			ChangeSignature(_chanCmd);
		}
		m_changeEntityVec.clear();

		// 削除前にリリース処理を走らせる
		RunSystem(Engine::ECS::ESystemType::Release, 0.0f);
		// 解放処理がされたエンティティたちは削除予定に追加
		ForEach<ReleaseTag>(
			[this]
			(
				ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				ReleaseTag* a_releaseTag
				)
			{
				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					AddRemoveEntity(a_pChunk->entityData[_i]);
				}
			}
		);

		// エンティティの一括削除
		RemoveEntityStorage();

		// すべてのECS参照カウントをリセット
		Engine::Resource::ResourceManager::Instance().AllResetECSRefs();

		// ECSカウントの収集
		RunSystem(Engine::ECS::ESystemType::GC, 0.0f);

		// 参照カウントがなくなった場合リソースの解放をする
		Engine::Resource::ResourceManager::Instance().RunGarbageCollectionSweep();

		ENGINE_LOG("Worldの解放");
	}

	void World::ClearMemory()
	{
		m_entityManager.Init();
	}

	void World::BeginFrame()
	{
		// 階層の変更通知をリセット
		auto& _res = GetResource<HierarchyResource>();
		_res.isDirty = false;

		// システムのソート
		m_systemManager.Sort();

		// エンティティの一括作成
		CreateAllEntity();
		// ---------------------------------------------------------
		// エンティティの引っ越し
		for (auto& _chanCmd : m_changeEntityVec)
		{
			ChangeSignature(_chanCmd);

			// エンティティの変更があったため階層の変更を通知する
			_res.isDirty = true;
		}
		m_changeEntityVec.clear();
		// ---------------------------------------------------------
		// 削除前にリリース処理を走らせる
		RunSystem(Engine::ECS::ESystemType::Release, 0.0f);
		// 解放処理がされたエンティティたちは削除予定に追加
		ForEach<ReleaseTag>(
			[this]
			(
				ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				ReleaseTag* a_releaseTag
				)
			{
				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					AddRemoveEntity(a_pChunk->entityData[_i]);
				}
			}
		);

		// エンティティの一括削除
		RemoveEntityStorage();

		// エンティティ削除後にエンティティをリフレッシュ
		RefreshEntities();

		// ---------------------------------------------------------
		// 初期化システムズ
		RunSystem(Engine::ECS::ESystemType::PostDeserialize, 0.0f);
		TransitionPhase<PostDeserializeTag, AwakeTag>();
	
		RunSystem(Engine::ECS::ESystemType::Awake, 0.0f);
		TransitionPhase<AwakeTag, StartTag>();

		RunSystem(Engine::ECS::ESystemType::Start, 0.0f);
		TransitionPhase<StartTag, ActiveTag>();
	}

	void World::ResourceGC()
	{

	}

	void World::AddEntity(const Signature& a_sig)
	{
		m_addEntityVec.push_back(a_sig);
	}

	void World::AddEntityWithData(const Signature& a_sig, std::unordered_map<ComponentTypeID, std::vector<uint8_t>> a_dataMap)
	{
		CreateEntityWithDataCmd _cmd = {};
		_cmd.sig = a_sig;
		_cmd.dataMap = std::move(a_dataMap);
		m_addEntityDataVec.push_back(std::move(_cmd));
	}

	ECS::Entity World::CreateEntity(const ECS::Signature& a_sig)
	{
		// エンティティIDの生成
		Signature _sig = a_sig;
		_sig.set(GetCompTypeID<PostDeserializeTag>());		// 初めて通るシステムフェーズ

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
			GetCompFunc(_i).construct(_data);
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

	bool World::HasComponent(const Entity& a_entity, const std::type_index& a_typeid)
	{
		auto _compType = m_componentMetaRegistry.GetTypeID(a_typeid);
		return HasComponent(a_entity,_compType);
	}

	bool World::HasComponent(const Entity& a_entity, const ComponentTypeID& a_comptype)
	{
		auto _sig = m_entityManager.GetSignature(a_entity);
		return _sig.test(a_comptype);
	}

	void World::CreateAllEntity()
	{
		for (auto& _sig : m_addEntityVec)
		{
			CreateEntity(_sig);

			// エンティティの追加があったため階層の変更を通知する
			auto& _res = GetResource<HierarchyResource>();
			_res.isDirty = true;
		}
		m_addEntityVec.clear();

		// データ付き生成(プレハブ実体化など)
		for (auto& _cmd : m_addEntityDataVec)
		{
			Entity _entity = CreateEntity(_cmd.sig);
			if (_entity == ECS::Limits::INVALID_ENTITY) continue;

			// 保存済みの初期値を各コンポーネントへ流し込む
			for (auto& [_compID, _buffer] : _cmd.dataMap)
			{
				if (!_cmd.sig.test(_compID)) continue;
				if (_buffer.empty()) continue;

				uint8_t* _dst = NRefData(_entity, _compID);
				if (!_dst) continue;

				size_t _size = GetComponentMetaData(_compID).compSize;
				size_t _copy = (_size < _buffer.size()) ? _size : _buffer.size();
				memcpy(_dst, _buffer.data(), _copy);
			}

			// 階層の変更を通知
			auto& _res = GetResource<HierarchyResource>();
			_res.isDirty = true;
		}
		m_addEntityDataVec.clear();
	}

	void World::RemoveEntityStorage()
	{
		// 消去予定エンティティがなければスキップ
		if (m_removeEntityVec.size() == 0) return;

		// ストレージにあるのは消去
		for (auto& _entity : m_removeEntityVec)
		{
			RemoveEntity(_entity);

			// エンティティの追加があったため階層の変更を通知する
			auto& _res = GetResource<HierarchyResource>();
			_res.isDirty = true;
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

	void World::AddComponent(ComponentTypeID a_typeID, Entity a_entity,uint8_t* a_pData)
	{
		// エンティティのシグネチャを変更
		Signature _oldSig = m_entityManager.GetSignature(a_entity);
		// 新たにシグネチャを作成
		if (_oldSig.test(a_typeID)) return;		// すでに持っていたらリターン
		_oldSig.set(a_typeID);

		// 命令の発行
		ChangeEntityCmd	_cmd = {};
		_cmd.entity = a_entity;
		if (_oldSig.test(GetCompTypeID<ActiveTag>()))
		{
			_oldSig.set(GetCompTypeID<PostDeserializeTag>());
			_oldSig.reset(GetCompTypeID<ActiveTag>());
		}
		_cmd.toSig = _oldSig;

		// 初期化データはディープコピーして保持
		if(a_pData)
		{
			// サイズ分コピー
			size_t _size = m_componentMetaRegistry.GetMetaData(a_typeID).compSize;
			_cmd.dataMap[a_typeID] = std::vector<uint8_t>(a_pData, a_pData + _size);
		}
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

	void World::ChangeSignature(ChangeEntityCmd a_cmd)
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
				memcpy(_pData, _it->second.data(), GetComponentMetaData(_compID).compSize);
			}
		}
	}

	void World::AddRefreshEntity(const Entity& a_entity)
	{
		// 無効エンティティはリフレッシュ経路(GetSignature→GetLocation)で
		// レンジ外参照になるため弾く。プレハブ編集など実体が無い呼び出し対策。
		if (a_entity == ECS::Limits::INVALID_ENTITY) return;

		m_refreshEntityVec.push_back(a_entity);
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
		// システムへ渡すコンテキストを組み立てる。
		// システム側は無捕獲(ステートレス)にして、必要な参照はすべてここから取る。
		SystemContext _context = {};
		_context.pWorld = this;
		_context.pServices = &m_engineServices;
		_context.dt = a_dt;

		m_systemManager.RunSystem(a_type, _context);
	}

	const std::unordered_map<ESystemType, std::vector<SystemTask*>>& World::GetCompileTaskMap() const
	{
		return m_systemManager.GetCompileTaskMap();
	}

	void World::RefreshEntities()
	{
		// 頻繁に呼ばれることはない想定なのでfor分内のエンティティを処理するのみ
		// リリースタグの付与
		for (auto& _entity : m_refreshEntityVec)
		{
			auto _sig = GetSignature(_entity);
			if (_sig.test(GetCompTypeID<ActiveTag>()))
			{
				_sig.reset(GetCompTypeID<ActiveTag>());
			}
			_sig.set(GetCompTypeID<ReleaseTag>());
			ChangeEntityCmd _cmd = {};
			_cmd.entity = _entity;
			_cmd.toSig = _sig;
			ChangeSignature(_cmd);
		}

		// リリース処理
		RunSystem(Engine::ECS::ESystemType::Release, 0.0f);

		// リリースされたものを初期化処理に回す
		TransitionPhase<ReleaseTag,PostDeserializeTag>();

		// コマンドクリア
		m_refreshEntityVec.clear();
	}

	World::World()
	{}

	World::~World()
	{}

}