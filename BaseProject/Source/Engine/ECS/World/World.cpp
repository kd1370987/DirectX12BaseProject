#include "World.h"

#include "../Internal/EntityLocation.h"


namespace Engine::ECS
{

	void World::Init()
	{
		// エンティティの置き場
		m_storage.Init(&m_componentMetaRegistry, &m_engineServices);

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
		ApplyReservedChange();

		// 生きているエンティティを全部削除予定へ積む。
		// 借りているものは RemoveEntity が解放フックを呼んで返す
		for (const auto& _loca : m_storage.GetAllEntityLocation())
		{
			if (!_loca.pChunk) continue;
			ReserveRemoveEntity(_loca.pChunk->entityData[_loca.chunkIndex]);
		}

		// エンティティの一括削除
		ApplyReservedRemove();

		ENGINE_LOG("Worldの解放");
	}

	//======================================================================================
	// フレームの先頭処理
	//--------------------------------------------------------------------------------------
	// 基盤がやるのは「1フレームの間に積まれた命令を捌く」ところまで。
	//   システムのソート → 生成 → 引っ越し → 削除 → リフレッシュ
	//
	// 初期化フェーズを進めるといったライフサイクルの決めごとは持たないので、
	// それを持つ層(App::ECS::APPWorld)が override して間に差し込むこと。
	//======================================================================================
	void World::BeginFrame()
	{
		// システムのソート
		m_systemManager.Sort();

		// エンティティの一括作成
		ApplyReservedCreate();

		// エンティティの引っ越し
		ApplyReservedChange();

		// エンティティの一括削除
		ApplyReservedRemove();

		// 作り直しに回されたものを流す
		ApplyReservedRefresh();
		ApplyReservedChange();
	}

	//======================================================================================
	// 溜まっているシグネチャ変更を今すぐ反映する
	//--------------------------------------------------------------------------------------
	// TransitionPhase は ForEach の最中に呼ばれるのでその場ではアーキタイプを動かせず、
	// 変更を予約する。反復が終わった直後にこれを呼んで流し込む。
	// 反復中に呼ぶとチャンクの並びが変わるので不可。
	//======================================================================================
	void World::ApplyReservedChange()
	{
		if (!m_commandBuffer.HasChange()) return;

		for (const auto& _changeCmd : m_commandBuffer.TakeChange())
		{
			ChangeSignature(_changeCmd);
		}

		// エンティティの構成が変わったことを派生へ知らせる
		OnEntityStructureChanged();
	}

	void World::ReserveCreateEntity(const Signature& a_sig)
	{
		m_commandBuffer.ReserveCreate(a_sig);
	}

	void World::ReserveCreateEntityWithData(const Signature& a_sig, ComponentDataMap a_dataMap)
	{
		m_commandBuffer.ReserveCreateWithData({
			.sig = a_sig,
			.dataMap = std::move(a_dataMap),
		});
	}

	ECS::Entity World::CreateEntity(const ECS::Signature& a_sig)
	{
		Signature _sig = a_sig;

		// 生まれた直後に何を載せるかは派生が決める(初期化フェーズのタグなど)
		OnCreateEntitySignature(_sig);

		return m_storage.Create(_sig);
	}

	const std::vector<EntityLocation>& World::GetEntityList()
	{
		return m_storage.GetAllEntityLocation();
	}

	const EntityLocation& World::GetLocation(const ECS::Entity& a_entity)
	{
		return m_storage.GetLocation(a_entity);
	}

	UINT World::GetAliveEntityCount()
	{
		return m_storage.GetAliveEntityCount();
	}

	bool World::IsAliveEntity(const ECS::Entity& a_entity)
	{
		return m_storage.IsAlive(a_entity);
	}

	const ECS::Entity& World::GetEntity(const EntityLocation& a_location)
	{
		if (!a_location.pChunk) return ECS::Limits::INVALID_ENTITY;

		return a_location.pChunk->entityData[a_location.chunkIndex];
	}

	ECS::Signature World::GetSignature(const ECS::Entity& a_entity)
	{
		return m_storage.GetSignature(a_entity);
	}

	bool World::HasComponent(const Entity& a_entity, const std::type_index& a_typeid)
	{
		auto _compType = m_componentMetaRegistry.GetTypeID(a_typeid);
		return HasComponent(a_entity,_compType);
	}

	bool World::HasComponent(const Entity& a_entity, const ComponentTypeID& a_comptype)
	{
		// 未登録の型は誰も持っていない。
		// INVALID(=255)のまま test するとシグネチャの範囲外で例外になる
		if (!IsValidTypeID(a_comptype)) return false;

		return m_storage.GetSignature(a_entity).test(a_comptype);
	}

	void World::ApplyReservedCreate()
	{
		for (const auto& _sig : m_commandBuffer.TakeCreate())
		{
			CreateEntity(_sig);

			// エンティティの構成が変わったことを派生へ知らせる
			OnEntityStructureChanged();
		}

		// データ付き生成(プレハブ実体化など)
		for (const auto& _cmd : m_commandBuffer.TakeCreateWithData())
		{
			Entity _entity = CreateEntity(_cmd.sig);
			if (_entity == ECS::Limits::INVALID_ENTITY) continue;

			// 保存済みの初期値を各コンポーネントへ流し込む
			m_storage.WriteComponentData(_entity, _cmd.dataMap);

			// エンティティの構成が変わったことを派生へ知らせる
			OnEntityStructureChanged();
		}
	}

	void World::ApplyReservedRemove()
	{
		for (const auto& _entity : m_commandBuffer.TakeRemove())
		{
			RemoveEntity(_entity);

			// エンティティの構成が変わったことを派生へ知らせる
			OnEntityStructureChanged();
		}
	}

	void World::ReserveRemoveEntity(const ECS::Entity& a_entity)
	{
		m_commandBuffer.ReserveRemove(a_entity);
	}

	//======================================================================================
	// エンティティの解放予約
	//--------------------------------------------------------------------------------------
	// 基盤は次の BeginFrame で消すだけ。借りているものは RemoveEntity が
	// 解放フックを呼んで返す。
	//
	// 消える前に後始末のフェーズを通したい層は override すること。
	//======================================================================================
	void World::ReserveReleaseEntity(const ECS::Entity& a_entity)
	{
		if (a_entity == ECS::Limits::INVALID_ENTITY) return;

		ReserveRemoveEntity(a_entity);
	}

	void World::RemoveEntity(const ECS::Entity& a_entity)
	{
		// 借りているものは EntityStorage が解放フックを呼んで返す
		m_storage.Destroy(a_entity);
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

	void World::ReserveAddComponent(ComponentTypeID a_typeID, Entity a_entity,uint8_t* a_pData)
	{
		// エンティティのシグネチャを変更
		Signature _oldSig = m_storage.GetSignature(a_entity);
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
			// インスペクタの ReserveAddComponent はデータを渡さないので、必ずここを通る。
			auto _construct = GetCompFunc(a_typeID).construct;
			if (_construct)
			{
				std::vector<uint8_t> _buffer(_size);
				_construct(_buffer.data());
				_cmd.dataMap[a_typeID] = std::move(_buffer);
			}
		}
		m_commandBuffer.ReserveChange(std::move(_cmd));
	}

	void World::ReserveRemoveComponent(ComponentTypeID a_typeID, Entity a_entity)
	{
		// エンティティのシグネチャを変更
		Signature _oldSig = m_storage.GetSignature(a_entity);

		// 新たにシグネチャを作成
		if (!_oldSig.test(a_typeID)) return;	// 持っていなければコマンドを発行しない
		_oldSig.reset(a_typeID);

		ReserveChangeSignature({
			.entity = a_entity,
			.toSig = _oldSig,
		});
	}

	void World::ReserveChangeSignature(ChangeEntityCmd a_cmd)
	{
		m_commandBuffer.ReserveChange(std::move(a_cmd));
	}

	void World::ChangeSignature(const ChangeEntityCmd& a_cmd)
	{
		// 予約した後に消えたエンティティ(古いID)は動かしようがない
		if (!m_storage.IsAlive(a_cmd.entity)) return;

		// 初期化へ入り直すなら、直後に取り直されるので今持っているものは全部返させる。
		// 入り直すかどうかの判断は派生が持つ(基盤はフェーズを知らない)
		const bool _isReenteringInit = IsReenteringInit(m_storage.GetSignature(a_cmd.entity), a_cmd.toSig);

		m_storage.Move(a_cmd.entity, a_cmd.toSig, a_cmd.dataMap, _isReenteringInit);
	}

	void World::ReserveRefreshEntity(const Entity& a_entity)
	{
		// 無効エンティティはリフレッシュ経路(GetSignature→GetLocation)で
		// レンジ外参照になるため弾く。プレハブ編集など実体が無い呼び出し対策。
		if (a_entity == ECS::Limits::INVALID_ENTITY) return;

		m_commandBuffer.ReserveRefresh(a_entity);
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
		return m_storage.RefComponent(a_entity, m_componentMetaRegistry.GetTypeID(a_index));
	}

	uint8_t* World::NRefData(const ECS::Entity& a_entity, const ECS::ComponentTypeID& a_typeID)
	{
		return m_storage.RefComponent(a_entity, a_typeID);
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
	// 初期化フェーズを持つ層(App::ECS::APPWorld)が override して、
	// 後始末を通してから初期化へ戻す。
	//======================================================================================
	void World::ApplyReservedRefresh()
	{
		m_commandBuffer.TakeRefresh();
	}

	World::World()
	{}

	World::~World()
	{}

}