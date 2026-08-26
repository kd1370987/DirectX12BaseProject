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

	//======================================================================================
	// ワールドの解放
	//--------------------------------------------------------------------------------------
	// エンティティを全部消す。コンポーネントが借りているリソースは、
	// 消すときに解放フック(ComponentTraits<T>::Release)が必ず返すので、
	// ここでリソースを数え直す必要はない。
	//
	// 参照が 0 になった実体を捨てるのはシーンの切れ目
	// (SceneManager::PopScene から ResourceManager::SweepUnusedAll)。
	//======================================================================================
	void World::Release()
	{
		// 積まれたままの引っ越しを片付けてから消す
		ApplyChangeSignatures();

		// 生きているエンティティを全部削除予定へ積む。
		// 借りているものは RemoveEntity が解放フックを呼んで返す
		for (const auto& _loca : m_entityManager.GetAllEntityLocation())
		{
			if (!_loca.pArchetypeChunk) continue;
			AddRemoveEntity(_loca.pArchetypeChunk->entityData[_loca.chunkIndex]);
		}

		// エンティティの一括削除
		RemoveEntityStorage();

		ENGINE_LOG("Worldの解放");
	}

	void World::ClearMemory()
	{
		m_entityManager.Init();
	}

	//======================================================================================
	// フレームの先頭処理
	//--------------------------------------------------------------------------------------
	// 基盤がやるのは「1フレームの間に積まれた命令を捌く」ところまで。
	//   システムのソート → 生成 → 引っ越し → 削除 → リフレッシュ
	//
	// 初期化フェーズを進めるといったライフサイクルの決めごとは持たないので、
	// それを持つ層(App::ECS::World)が override して間に差し込むこと。
	//======================================================================================
	void World::BeginFrame()
	{
		// システムのソート
		m_systemManager.Sort();

		// エンティティの一括作成
		CreateAllEntity();

		// エンティティの引っ越し
		ApplyChangeSignatures();

		// エンティティの一括削除
		RemoveEntityStorage();

		// 作り直しに回されたものを流す
		RefreshEntities();
		ApplyChangeSignatures();
	}

	//======================================================================================
	// 溜まっているシグネチャ変更を今すぐ反映する
	//--------------------------------------------------------------------------------------
	// TransitionPhase は ForEach の最中に呼ばれるのでその場ではアーキタイプを動かせず、
	// 変更を予約する。反復が終わった直後にこれを呼んで流し込む。
	// 反復中に呼ぶとチャンクの並びが変わるので不可。
	//======================================================================================
	void World::ApplyChangeSignatures()
	{
		if (m_changeEntityVec.empty()) return;

		for (auto& _chanCmd : m_changeEntityVec)
		{
			ChangeSignature(_chanCmd);
		}
		m_changeEntityVec.clear();

		// エンティティの構成が変わったことを派生へ知らせる
		OnEntityStructureChanged();
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

		// 生まれた直後に何を載せるかは派生が決める(初期化フェーズのタグなど)
		OnCreateEntitySignature(_sig);

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

	bool World::IsAliveEntity(const ECS::Entity& a_entity)
	{
		return m_entityManager.IsAlive(a_entity);
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

			// エンティティの構成が変わったことを派生へ知らせる
			OnEntityStructureChanged();
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

			// エンティティの構成が変わったことを派生へ知らせる
			OnEntityStructureChanged();
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

			// エンティティの構成が変わったことを派生へ知らせる
			OnEntityStructureChanged();
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

	//======================================================================================
	// エンティティの解放予約
	//--------------------------------------------------------------------------------------
	// 基盤は次の BeginFrame で消すだけ。借りているものは RemoveEntity が
	// 解放フックを呼んで返す。
	//
	// 消える前に後始末のフェーズを通したい層は override すること。
	//======================================================================================
	void World::AddReleaseEntity(const ECS::Entity& a_entity)
	{
		if (a_entity == ECS::Limits::INVALID_ENTITY) return;

		AddRemoveEntity(a_entity);
	}

	void World::RemoveEntity(const ECS::Entity& a_entity)
	{
		// ロケーション取得
		const auto& _loca = m_entityManager.GetLocation(a_entity);
		if (!_loca.pArchetypeChunk)return;

		// 消える前に、コンポーネントが借りているものを返させる。
		// コンポーネントはデストラクタが走らない(trivially copyable 縛り)ので、
		// リソースの参照カウントはここで返さないと戻らない
		ReleaseComponents(a_entity, m_entityManager.GetSignature(a_entity));

		// アーキタイプから削除して、移動したエンティティの情報をもらう
		auto [_entity, _idx] = m_archetypeChunkManager.RemoveEntity(_loca);

		// エンティティマネージャーからも消去
		m_entityManager.DestroyEntity(a_entity);

		// 移動したエンティティのロケーションを変更
		auto& _swapLoca = m_entityManager.RefEntityLocation(_entity);
		_swapLoca.chunkIndex = _idx;
	}

	//======================================================================================
	// コンポーネントが借りているものを返させる
	//--------------------------------------------------------------------------------------
	// ComponentTraits<T>::Release を書いてあるコンポーネントだけが対象。
	// 解放フックはハンドルを空にするので、返したものを持ち主のふりで持ち続けない。
	//======================================================================================
	void World::ReleaseComponents(const ECS::Entity& a_entity, const Signature& a_sig)
	{
		for (ComponentTypeID _compID = 0; _compID < a_sig.size(); ++_compID)
		{
			if (!a_sig.test(_compID)) continue;

			const auto& _release = GetCompFunc(_compID).release;
			if (!_release) continue;

			if (uint8_t* _pData = NRefData(a_entity, _compID))
			{
				_release(_pData);
			}
		}
	}

	//======================================================================================
	// 退避したコンポーネントのデータに対して解放フックを呼ぶ
	//--------------------------------------------------------------------------------------
	// アーキタイプの引っ越し中は実体の置き場所が変わるので、退避したバッファを直接渡す。
	// 引っ越し先へ書き戻されるのはこのバッファなので、空にした結果もそのまま伝わる。
	//======================================================================================
	void World::ReleaseComponentData(ComponentTypeID a_compID, uint8_t* a_pData)
	{
		if (!a_pData) return;

		const auto& _release = GetCompFunc(a_compID).release;
		if (!_release) return;

		_release(a_pData);
	}

	//======================================================================================
	// GUIDからエンティティを探す
	//--------------------------------------------------------------------------------------
	// 基盤のエンティティは「番号」でしかなく、保存をまたいで残る識別子は持たない。
	// GUIDを載せるコンポーネントを定義した層が override して探す。
	//======================================================================================
	Entity World::GetEntity(const Engine::GUID& a_guid)
	{
		(void)a_guid;
		return Limits::INVALID_ENTITY;
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

		// 構成が変わったので初期化からやり直させる(判断と中身は派生が持つ)
		OnReenterInitSignature(_oldSig);

		_cmd.toSig = _oldSig;

		// 初期化データはディープコピーして保持
		const size_t _size = m_componentMetaRegistry.GetMetaData(a_typeID).compSize;
		if(a_pData)
		{
			// サイズ分コピー
			_cmd.dataMap[a_typeID] = std::vector<uint8_t>(a_pData, a_pData + _size);
		}
		else
		{
			// 初期値が渡されなかった場合は既定値で構築しておく。
			//
			// ここを空のままにすると、チャンクの生メモリがそのまま新しいコンポーネントになり、
			// C++側のメンバ初期化子(ModelComponent::emissiveScale = {1,1,1} など)が
			// 一切効かないままゼロ値で始まってしまう。
			// インスペクタの AddComponent はデータを渡さないので、必ずここを通る。
			auto _construct = GetCompFunc(a_typeID).construct;
			if (_construct)
			{
				std::vector<uint8_t> _buffer(_size);
				_construct(_buffer.data());
				_cmd.dataMap[a_typeID] = std::move(_buffer);
			}
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
		m_changeEntityVec.push_back(std::move(a_cmd));
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

		//------------------------------------------------------------------
		// 借りているものを返させる
		//------------------------------------------------------------------
		// 対象は次の2つ。どちらも退避したバッファに対して呼ぶので、
		// ハンドルを空にした結果は引っ越し先へそのまま伝わる。
		//
		//   ・外されるコンポーネント     : この先持ち主がいなくなる
		//   ・初期化へ入り直すエンティティ : 直後に取り直されるので、
		//                                ここで返さないと二重に持つことになる
		//
		// 「初期化へ入り直すかどうか」の判断は派生が持つ(基盤はフェーズを知らない)
		//------------------------------------------------------------------
		const bool _isBackToFixup = IsReenteringInit(_oldSig, a_cmd.toSig);

		for (auto& [_compID, _buffer] : _oldData)
		{
			const bool _isRemoved = !a_cmd.toSig.test(_compID);

			// 初期値で上書きされるものも、今持っているぶんは返す
			const bool _isOverwritten = (a_cmd.dataMap.find(_compID) != a_cmd.dataMap.end());

			if (!_isRemoved && !_isBackToFixup && !_isOverwritten) continue;

			ReleaseComponentData(_compID, _buffer.data());
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
		if (!_loca.pArchetypeChunk) return nullptr;
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

	//======================================================================================
	// リフレッシュ(作り直し)の消化
	//--------------------------------------------------------------------------------------
	// 基盤には「作り直す」という工程が無いので、積まれたものを捨てるだけ。
	// 初期化フェーズを持つ層(App::ECS::World)が override して、
	// 後始末を通してから初期化へ戻す。
	//======================================================================================
	void World::RefreshEntities()
	{
		m_refreshEntityVec.clear();
	}

	World::World()
	{}

	World::~World()
	{}

}