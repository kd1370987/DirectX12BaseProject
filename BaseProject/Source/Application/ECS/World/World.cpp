#include "World.h"

// エンティティに初めからつけるもの
#include "../../Components/Persistence/GUIDComponent.h"		// GUID
#include "../../Components/Hierarchy/HierarchyComponent.h"	// 親子関係(解放を子へ広げるのに使う)

// シングルトンリソース
#include "../../InstanceResource/HierarchyResource.h"		// 階層保持
#include "../../InstanceResource/ResourceWaitResource.h"	// リソース到着待ち

// ゲーム固有の型登録
#include "WorldTypeRegister.h"

namespace App::ECS
{
	//======================================================================================
	// 自分が前提にしているリソースを確保しておく
	//--------------------------------------------------------------------------------------
	// BeginFrame / OnEntityStructureChanged が毎フレーム引くので、
	// 登録し忘れると成立しない。持ち主が使う側と同じなら、確保もここでやる。
	//======================================================================================
	World::World()
	{
		AddResource<HierarchyResource>();
		AddResource<ResourceWaitResource>();
	}

	void World::RegisterGameTypes()
	{
		App::ECS::RegisterGameTypes(*this);
	}

	//======================================================================================
	// フレームの先頭処理
	//--------------------------------------------------------------------------------------
	// PostDeserialize -> Awake -> Start -> Active をこの1回で通しきる。
	// 各フェーズの遷移ごとに引っ越しを流し込むのがその要。
	//
	// TransitionPhase はタグの張り替えを「予約」するだけなので、流さないと反映が
	// 次の BeginFrame になる。つまりフェーズが1段進むのに1フレームかかり、
	// 生成命令を出してから描画されるまで4フレーム待たされていた
	// (弾やエフェクトが遅れて見える原因)。
	//
	// システムが実行中に足したコンポーネントも、遷移の前に流しておくこと。
	// TransitionPhase は張り替え先をその場のシグネチャから作るので、予約が
	// 残っていると後から流したほうに上書きされて消える。
	//======================================================================================
	void World::BeginFrame()
	{
		// 階層の変更通知をリセット
		GetResource<HierarchyResource>().isDirty = false;

		// システムのソート
		m_systemManager.Sort();

		// エンティティの一括作成
		CreateAllEntity();

		// エンティティの引っ越し
		ApplyChangeSignatures();

		// 解放されるものの子孫にもタグを広げる。
		// 引っ越しが済んだ後に呼ぶので、この時点で親にはもうタグが付いている。
		// ここで広げておけば、親子ともに同じフレームの Release フェーズを通ってから消える
		PropagateReleaseToChildren();

		// 削除前にリリース処理を走らせる
		RunSystem(ESystemType::Release, 0.0f);
		CollectReleasedEntities();

		// エンティティの一括削除
		RemoveEntityStorage();

		// エンティティ削除後にエンティティをリフレッシュ
		// 作り直しに回されたものは PostDeserializeTag が予約されるだけなので、
		// ここで流しておかないと下の初期化フェーズに乗り遅れて1フレーム待たされる
		RefreshEntities();
		ApplyChangeSignatures();

		// ---------------------------------------------------------
		// 初期化システムズ
		// ---------------------------------------------------------
		RunSystem(ESystemType::PostDeserialize, 0.0f);
		ApplyChangeSignatures();
		TransitionPhase<PostDeserializeTag, AwakeTag>();
		ApplyChangeSignatures();

		RunSystem(ESystemType::Awake, 0.0f);
		ApplyChangeSignatures();

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

			ApplyChangeSignatures();

			// 判定はフレームごとにやり直す
			_waitRes.waitingEntities.clear();
		}

		RunSystem(ESystemType::Start, 0.0f);
		ApplyChangeSignatures();
		TransitionPhase<StartTag, ActiveTag>();
		ApplyChangeSignatures();
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
		// 動いているものを後始末へ回す
		TransitionPhase<ActiveTag, ReleaseTag>();
		ApplyChangeSignatures();

		// 削除前にリリース処理を走らせる
		RunSystem(ESystemType::Release, 0.0f);
		CollectReleasedEntities();

		// エンティティの一括削除
		RemoveEntityStorage();

		// 後始末を通さずに残っているもの(Release フェーズへ乗らなかったもの)を片付ける
		Base::Release();
	}

	//======================================================================================
	// エンティティの解放予約
	//======================================================================================
	void World::AddReleaseEntity(const Entity& a_entity)
	{
		if (a_entity == Engine::ECS::Limits::INVALID_ENTITY) return;

		Signature _sig = GetSignature(a_entity);

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

	//======================================================================================
	// GUIDからエンティティを探す
	//======================================================================================
	Entity World::GetEntity(const Engine::GUID& a_guid)
	{
		Entity _res = Engine::ECS::Limits::INVALID_ENTITY;

		ForEach<GUIDComponent>(
			[&a_guid, &_res](
				ArchetypeChunk* a_chunk,
				uint32_t a_count,
				GUIDComponent* a_guidArray
				)
			{
				if (_res != Engine::ECS::Limits::INVALID_ENTITY) return;

				for (size_t _i = 0; _i < a_count; ++_i)
				{
					if (_res != Engine::ECS::Limits::INVALID_ENTITY) continue;

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

	//======================================================================================
	// 基盤から呼ばれるフック
	//======================================================================================
	void World::OnCreateEntitySignature(Signature& a_sig)
	{
		// 初めて通るシステムフェーズ
		a_sig.set(GetCompTypeID<PostDeserializeTag>());

		// 保存データに ActiveTag が入っていても、初期化を飛ばさせない
		if (a_sig.test(GetCompTypeID<ActiveTag>()))
		{
			a_sig.reset(GetCompTypeID<ActiveTag>());
		}
	}

	void World::OnReenterInitSignature(Signature& a_sig)
	{
		// 動いているものだけを初期化へ戻す。
		// まだ初期化中のものは、今いるフェーズをそのまま続けさせる
		if (!a_sig.test(GetCompTypeID<ActiveTag>())) return;

		a_sig.set(GetCompTypeID<PostDeserializeTag>());
		a_sig.reset(GetCompTypeID<ActiveTag>());
	}

	bool World::IsReenteringInit(const Signature& a_from, const Signature& a_to)
	{
		// PostDeserialize へ入り直すなら、直後に fixup が取り直すので
		// 今持っているものは返させる(返さないと二重に持つ)
		const ComponentTypeID _postDeserializeID = GetCompTypeID<PostDeserializeTag>();
		return a_to.test(_postDeserializeID) && !a_from.test(_postDeserializeID);
	}

	void World::OnEntityStructureChanged()
	{
		// エンティティの構成が変わったので階層の作り直しを促す
		GetResource<HierarchyResource>().isDirty = true;
	}

	//======================================================================================
	// リフレッシュ(作り直し)の消化
	//--------------------------------------------------------------------------------------
	// 後始末を通してから初期化フェーズへ戻す。
	// モデルの差し替えなど、借りているものを取り直す必要がある編集で使う。
	//======================================================================================
	void World::RefreshEntities()
	{
		// 頻繁に呼ばれることはない想定なので、溜まったぶんをそのまま回す
		for (const Entity& _entity : m_refreshEntityVec)
		{
			Signature _sig = GetSignature(_entity);
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
		RunSystem(ESystemType::Release, 0.0f);

		// リリースされたものを初期化処理に回す
		TransitionPhase<ReleaseTag, PostDeserializeTag>();

		// コマンドクリア
		m_refreshEntityVec.clear();
	}

	//======================================================================================
	// ReleaseTag が付いているものを削除予定へ積む
	//======================================================================================
	void World::CollectReleasedEntities()
	{
		ForEach<ReleaseTag>(
			[this]
			(
				ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				ReleaseTag* a_releaseTag
				)
			{
				(void)a_releaseTag;
				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					AddRemoveEntity(a_pChunk->entityData[_i]);
				}
			}
		);
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
		if (_releaseTypeID == Engine::ECS::Limits::INVALID_COMPONENTTYPEID) return;

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
				(void)a_releaseTag;
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
						if (_parent == Engine::ECS::Limits::INVALID_ENTITY) continue;

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
			Signature _sig = GetSignature(_child);

			// もう動かす必要はないので Active から外して Release へ移す。
			// この後すぐ Release フェーズが走るので、借りているものは返ってから消える
			if (_activeTypeID != Engine::ECS::Limits::INVALID_COMPONENTTYPEID)
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
		OnEntityStructureChanged();
	}
}
