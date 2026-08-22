#include "World.h"

#include "../Internal/EntityLocation.h"

// エンティティに初めからつけるためこの二つはインクルード
#include "../../../Application/Components/Persistence/GUIDComponent.h"		// GUID
#include "../../../Application/Components/Persistence/NameComponent.h"		// 名前
#include "../../../Application/Components/Hierarchy/HierarchyComponent.h"	// 親子関係(解放を子へ広げるのに使う)

// シングルトンリソース
#include "../../../Application/InstanceResource/HierarchyResource.h"		// 階層保持
#include "../../../Application/InstanceResource/ResourceWaitResource.h"	// リソース到着待ち


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
		// 解放されるものの子孫にもタグを広げる。
		// 引っ越しが済んだ後に呼ぶので、この時点で親にはもうタグが付いている。
		// ここで広げておけば、親子ともに同じフレームの Release フェーズを通ってから消える
		PropagateReleaseToChildren();
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

		// リソースが揃ったエンティティだけ Start へ進める。
		// 揃っていないものは AwakeTag のまま残り、次のフレームで再判定される。
		//
		// Start の中で個別にスキップしないのは、同じエンティティに
		// Start 系が複数ぶら下がっていて「一部だけ走った」状態を作れないため。
		// 領域確保をするシステムがあるので、二重実行はそのままリークになる
		{
			auto& _waitRes = GetResource<ResourceWaitResource>();

			TransitionPhase<AwakeTag, StartTag>(
				[&_waitRes](Entity a_entity)
				{
					return !_waitRes.IsWaiting(a_entity);
				}
			);

			// 判定はフレームごとにやり直す
			_waitRes.waitingEntities.clear();
		}

		RunSystem(Engine::ECS::ESystemType::Start, 0.0f);
		TransitionPhase<StartTag, ActiveTag>();
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

	//======================================================================================
	// 解放されるエンティティの子孫にも ReleaseTag を広げる
	//--------------------------------------------------------------------------------------
	// HierarchyComponent が持っているのは親だけなので、子は「親が誰か」を見て探す。
	// 親→子の対応表はどこにも無く、作っても親子が変わるたびに作り直しになるため、
	// 解放が起きたフレームだけ層ごとに舐める。
	//
	//   1周目 : 親が解放される子
	//   2周目 : その子が親になっている孫
	//   ...
	// 新しく見つからなくなったら終わり。すでに対象に入っているものは飛ばすので、
	// 親子が循環していても止まる。
	//
	// タグを付けるのは全部見終わってから。反復の最中に引っ越しをかけると
	// チャンクの中身が動いて、走査そのものが壊れる。
	//======================================================================================
	void World::PropagateReleaseToChildren()
	{
		const ComponentTypeID _releaseTypeID = GetCompTypeID<ReleaseTag>();
		const ComponentTypeID _activeTypeID  = GetCompTypeID<ActiveTag>();
		if (_releaseTypeID == Limits::INVALID_COMPONENTTYPEID) return;

		//----------------------------------------------------------------------
		// このフレームに解放されるもの
		//----------------------------------------------------------------------
		std::unordered_set<Entity> _releasing = {};

		ForEach<ReleaseTag>(
			[&_releasing]
			(
				ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				ReleaseTag* a_releaseTag
				)
			{
				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					_releasing.insert(a_pChunk->entityData[_i]);
				}
			}
		);

		if (_releasing.empty()) return;

		//----------------------------------------------------------------------
		// 子・孫…と辿って集める
		//----------------------------------------------------------------------
		// 深さの保険。親子が循環していなくてもここで必ず止まる
		constexpr int _kMaxDepth = 32;

		std::vector<Entity> _found = {};
		std::vector<Entity> _targets = {};

		for (int _depth = 0; _depth < _kMaxDepth; ++_depth)
		{
			_found.clear();

			ForEach<const HierarchyComponent>(
				[&_releasing, &_found]
				(
					ArchetypeChunk* a_pChunk,
					uint32_t a_count,
					const HierarchyComponent* a_hierarchyArray
					)
				{
					for (uint32_t _i = 0; _i < a_count; ++_i)
					{
						const Entity _parent = a_hierarchyArray[_i].parentID;
						if (_parent == Limits::INVALID_ENTITY) continue;

						// 親が解放されないなら、この子はそのまま残す
						if (!_releasing.contains(_parent)) continue;

						// すでに対象になっているものは数えない(循環よけも兼ねる)
						const Entity _child = a_pChunk->entityData[_i];
						if (_releasing.contains(_child)) continue;

						_found.push_back(_child);
					}
				}
			);

			// これ以上ぶら下がっていない
			if (_found.empty()) break;

			for (const Entity& _child : _found)
			{
				_releasing.insert(_child);
				_targets.push_back(_child);
			}
		}

		if (_targets.empty()) return;

		//----------------------------------------------------------------------
		// タグを付ける(走査が終わってから)
		//----------------------------------------------------------------------
		for (const Entity& _child : _targets)
		{
			Signature _sig = m_entityManager.GetSignature(_child);

			// もう動かす必要はないので Active から外して Release へ移す。
			// この後すぐ Release フェーズが走るので、借りているものは返ってから消える
			if (_activeTypeID != Limits::INVALID_COMPONENTTYPEID)
			{
				_sig.reset(_activeTypeID);
			}
			_sig.set(_releaseTypeID);

			ChangeEntityCmd _cmd = {};
			_cmd.entity = _child;
			_cmd.toSig = _sig;

			ChangeSignature(_cmd);
		}

		// 階層が変わったので通知しておく
		GetResource<HierarchyResource>().isDirty = true;
	}

	void World::AddReleaseEntity(const ECS::Entity& a_entity)
	{
		if (a_entity == ECS::Limits::INVALID_ENTITY) return;

		Signature _sig = m_entityManager.GetSignature(a_entity);

		// すでに解放待ちなら積み直さない(寿命と撃破が同じフレームに重なる等)
		if (_sig.test(GetCompTypeID<ReleaseTag>())) return;

		// もう動かす必要はないので Active から外して Release へ移す。
		// BeginFrame では「引っ越し → Release実行 → ReleaseTag付きを削除」の順に流れるので、
		// 次のフレームの頭で解放処理まで済ませて消える
		if (_sig.test(GetCompTypeID<ActiveTag>()))
		{
			_sig.reset(GetCompTypeID<ActiveTag>());
		}
		_sig.set(GetCompTypeID<ReleaseTag>());

		ChangeEntityCmd _cmd = {};
		_cmd.entity = a_entity;
		_cmd.toSig = _sig;

		AddChangeSigCommand(_cmd);
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
		//   ・外されるコンポーネント   : この先持ち主がいなくなる
		//   ・PostDeserialize へ入り直す : 直後に fixup が取り直すので、
		//                              ここで返さないと二重に持つことになる
		//------------------------------------------------------------------
		const ComponentTypeID _postDeserializeID = GetCompTypeID<PostDeserializeTag>();
		const bool _isBackToFixup =
			a_cmd.toSig.test(_postDeserializeID) && !_oldSig.test(_postDeserializeID);

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