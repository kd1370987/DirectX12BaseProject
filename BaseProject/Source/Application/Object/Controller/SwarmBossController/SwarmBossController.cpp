#include "SwarmBossController.h"

// エンジン
#include "Engine/ECS/Internal/SystemContext.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/Prefab/Prefab.h"
#include "Engine/Editor/Helper/EditorHelper.h"	// コンポーネントの Traits が使うので先に置く

// App
#include "../../../ECS/World/APPWorld.h"
#include "../../../Utility/PrefabSpawnHelper.h"

#include "../../../Components/Transform/LocalTransformComponent.h"
#include "../../../Components/Force/MovementComponent.h"
#include "../../../Components/Force/VelocityComponent.h"
#include "../../../Components/Persistence/GUIDComponent.h"
#include "../../../Components/Hierarchy/SpawnerComponent.h"
#include "../../../Components/Tag/SwarmBossBoidTag.h"
#include "../../../Components/Collision/Collider.h"
#include "../../../Components/Collision/SphereCollider.h"
#include "../../../Components/Character/HealthComponent.h"
#include "Engine/ECS/Internal/CollisionEvent.h"
#include "../../../Components/Character/BoidComponent.h"
#include "../../../Components/Character/LookAngleComponent.h"
#include "../../../Components/Intent/MoveIntentComponent.h"
#include "../../../Components/Character/Boss/BoidLeaderComponent.h"
#include "../../../Components/Character/Boss/PlatoonLeaderComponent.h"
#include "../../../Components/Character/Boss/BoidSpownerComponent.h"

namespace App::Object
{
	namespace
	{
		// 小隊長を並べる向き(リーダーの後ろ)。左手系 +Z 前方なので -Z。
		// 生成直後の LookAngle は既定(Yaw 0 = +Z 前方)なので、その後ろに並ぶ形になる
		const Math::Vector3 PLATOON_LINE_DIR = { 0.0f, 0.0f, -1.0f };

		//----------------------------------------------------------------------
		// 材料のルートにあるコンポーネントを書き換える。持っていなければ足してから渡す
		// (a_func を空にすれば「無ければ足す」だけになる)
		//----------------------------------------------------------------------
		template<typename T, typename TFunc>
		bool EditRootComponent(
			Engine::ECS::World& a_world,
			std::vector<Engine::Resource::PrefabInstanceData>& a_instanceVec,
			TFunc&& a_func)
		{
			if (a_instanceVec.empty()) return false;

			uint8_t* _pBuf = App::Utility::EnsureInstanceComponent(
				a_world, a_instanceVec[0], a_world.GetCompTypeID<T>());
			if (!_pBuf) return false;

			// バイト列の置き場はアライメントが揃っていないので、手元へ写してから触る
			T _comp = {};
			std::memcpy(&_comp, _pBuf, sizeof(T));
			a_func(_comp);
			std::memcpy(_pBuf, &_comp, sizeof(T));
			return true;
		}

		// 無ければ既定値で足すだけ
		template<typename T>
		bool EnsureRootComponent(
			Engine::ECS::World& a_world,
			std::vector<Engine::Resource::PrefabInstanceData>& a_instanceVec)
		{
			return EditRootComponent<T>(a_world, a_instanceVec, [](T&) {});
		}

		//----------------------------------------------------------------------
		// 移動速度を流し込む(加減速は速度から決める)
		//
		// 追従できるかは速さの配分で決まるので、プレハブの値ではなく
		// こちら(群れ全体の持ち主)が決めた値を入れる。
		// 加減速が小さいと最高速に乗る前に目標が変わってしまうので、速さに比例させる
		//----------------------------------------------------------------------
		void ApplyMoveSpeed(
			Engine::ECS::World& a_world,
			std::vector<Engine::Resource::PrefabInstanceData>& a_instanceVec,
			float a_speed)
		{
			EditRootComponent<MovementComponent>(a_world, a_instanceVec,
				[a_speed](MovementComponent& a_comp)
				{
					a_comp.moveSpeed    = a_speed;
					a_comp.acceleration = a_speed * 4.0f;
					a_comp.deceleration = a_speed * 4.0f;
				}
			);
		}

		//----------------------------------------------------------------------
		// 半径 a_radius の球内に一様に散らした点(中心からの相対)
		// 同じ座標に重ねるとボイドの反発の向きが出ないのでばらまく
		//----------------------------------------------------------------------
		Math::Vector3 RandomInSphere(float a_radius)
		{
			Math::Vector3 _dir(
				Math::Random::Float(-1.0f, 1.0f),
				Math::Random::Float(-1.0f, 1.0f),
				Math::Random::Float(-1.0f, 1.0f));
			if (_dir.LengthSquared() < 1e-6f) _dir = Math::Vector3(0.0f, 0.0f, 1.0f);
			_dir.Normalize();

			// 半径は立方根で偏りを消す(そのままだと中心に寄る)
			const float _r = a_radius * std::cbrt(Math::Random::Float(0.0f, 1.0f));
			return _dir * _r;
		}

		// エンティティのGUID(持っていなければ無効)
		Engine::GUID GetEntityGUID(Engine::ECS::World& a_world, Engine::ECS::Entity a_entity)
		{
			if (!a_world.HasComponent<GUIDComponent>(a_entity)) return Engine::DefaultGUID;
			return a_world.RefData<GUIDComponent>(a_entity)->guid;
		}

		//----------------------------------------------------------------------
		// プレハブを読み込んで実体を返す。GUIDが未設定なら nullptr
		//
		// ここは「参照して実体化するだけ」の経路。プレハブの中身は一切書き換えない
		// (BuildSpawnInstanceData が受け取るのは BuildInstanceData が作った複製)。
		//----------------------------------------------------------------------
		const Engine::Resource::Prefab* LoadPrefab(
			Engine::GameObject::ObjectContext& a_context,
			const Engine::GUID& a_guid,
			Engine::ResourceRef<Engine::Resource::Prefab>& a_inoutRef)
		{
			if (!a_guid.IsValid()) return nullptr;

			auto& _rm = *a_context.pServices->pResourceManager;

			// 生成に使うので実体ができるまで待つ。
			// 同じGUIDならキャッシュから同じハンドルが返る
			a_inoutRef = _rm.LoadImmediate<Engine::Resource::Prefab>(a_guid);

			const auto* _pPrefab = _rm.Get(a_inoutRef);
			if (!_pPrefab) return nullptr;

			// 中身が無いものは読み込みに失敗したものとして扱う。
			// 空のまま実体化すると、コンポーネントを持たないエンティティが
			// シーンに残る(そのまま保存されると余計なものが増える)
			if (_pPrefab->GetSignature().none())
			{
				ENGINE_WARNING("SwarmBossController : プレハブの中身が空です : %s", a_guid.String().c_str());
				return nullptr;
			}

			return _pPrefab;
		}
	}

	void SwarmBossController::PostDeserialize(Engine::GameObject::ObjectContext& a_context)
	{}
	void SwarmBossController::Awake(Engine::GameObject::ObjectContext& a_context)
	{
		// リーダー → 小隊長 → ボイドの順に生成
		Spawn(a_context);
	}
	void SwarmBossController::Start(Engine::GameObject::ObjectContext& a_context)
	{}
	void SwarmBossController::Update(Engine::GameObject::ObjectContext& a_context)
	{
		if (!m_isSpown || !a_context.pWorld) return;

		// リーダーの行動(入力を作る)
		UpdateLeaderBrain(a_context);

		// 残りの生存数(HP代わり)。印を数え直すだけ
		m_currentBoids = CountAliveBoids(a_context);
	}

	//======================================================================================
	// リーダーの行動 : ステートマシンを回す
	//--------------------------------------------------------------------------------------
	// このクラスはプレイヤーのキーボード/マウスと同じ立場で、作るのは移動入力だけ。
	// 何をするかは各ステート(SwarmBossStates)。切り替え要求は次のフレームの PreUpdate で反映
	//======================================================================================
	void SwarmBossController::UpdateLeaderBrain(Engine::GameObject::ObjectContext& a_context)
	{
		SwarmBossStateContext _stateContext = {};
		_stateContext.pObject      = &a_context;
		_stateContext.pMachine     = &m_stateMachine;
		_stateContext.leaderEntity = m_leaderEntity;
		_stateContext.spawnPos     = m_spawnPos;

		m_stateMachine.PreUpdate(_stateContext);
		m_stateMachine.Update(_stateContext);
		m_stateMachine.PostUpdate(_stateContext);
	}

	//======================================================================================
	// 生成
	//======================================================================================
	bool SwarmBossController::Spawn(Engine::GameObject::ObjectContext& a_context)
	{
		if (m_isSpown) return true;
		if (!a_context.pWorld || !a_context.pServices || !a_context.pServices->pResourceManager) return false;

		// リーダーが居なければ何も出さない。
		// 置いた直後(プレハブ未設定)もここを通るので、止めずに警告だけ出す
		if (!CreateLeader(a_context)) return false;

		// リーダーが出た時点で「出し済み」にする。
		// 小隊長やボイドで失敗しても、出したリーダーの上に二重に出さないため
		m_isSpown = true;

		// 小隊長 → 各小隊長のボイド
		return CreatePlatoonLeaders(a_context);
	}

	bool SwarmBossController::CreateLeader(Engine::GameObject::ObjectContext& a_context)
	{
		auto& _world = *a_context.pWorld;

		// リーダー生成
		const auto* _pPrefab = LoadPrefab(a_context, m_leaderPrefabGUID, m_leaderPrefabHandle);
		if (!_pPrefab)
		{
			ENGINE_WARNING("SwarmBossController : リーダー生成用のプレハブが設定されていません");
			return false;
		}

		// プレハブのインスタンス取得(位置はここで入る)
		App::Utility::SpawnParams _params = {};
		_params.pos = m_spawnPos;

		std::vector<Engine::Resource::PrefabInstanceData> _instanceVec = {};
		if (!App::Utility::BuildSpawnInstanceData(_world, *_pPrefab, _params, _instanceVec))
		{
			ENGINE_ERRLOG(false, "SwarmBossController : リーダー生成用のプレハブの取得に失敗");
			return false;
		}

		// リーダーに必須なコンポーネントを付与 : すでにあればスキップ
		// (LocalTransform は BuildSpawnInstanceData が足している)
		ApplyMoveSpeed(_world, _instanceVec, m_leaderSpeed);
		EnsureRootComponent<VelocityComponent>(_world, _instanceVec);
		EnsureRootComponent<BoidLeaderComponent>(_world, _instanceVec);

		// 移動入力の受け皿。中身を書くのはこのクラス(UpdateLeaderBrain)
		EnsureRootComponent<MoveIntentComponent>(_world, _instanceVec);

		// どちらを向いているか。進んでいる向きへ寄せるのは SwarmLookSystem、
		// 体の向きにするのは RotationSystem。既定は Yaw 0 = +Z 前方で、
		// 小隊長を並べる向き(PLATOON_LINE_DIR)と揃えてある。
		// 上下も体ごと向かせる(空を泳ぐので、人型のように上体だけでは向かない)
		EditRootComponent<LookAngleComponent>(_world, _instanceVec,
			[](LookAngleComponent& a_comp)
			{
				a_comp.isApplyPitchToBody = true;
			}
		);

		// リーダー生成
		m_leaderEntity = App::Utility::CreateInstanceNow(_world, _instanceVec);
		if (m_leaderEntity == Engine::ECS::Limits::INVALID_ENTITY)
		{
			ENGINE_ERRLOG(false, "SwarmBossController : リーダーのエンティティを作れませんでした");
			return false;
		}

		return true;
	}

	bool SwarmBossController::CreatePlatoonLeaders(Engine::GameObject::ObjectContext& a_context)
	{
		auto& _world = *a_context.pWorld;

		m_platoonLeaderEntities.clear();
		m_currentBoids = 0;

		if (m_maxPlatoonLeader == 0) return true;

		// プレハブ取得
		const auto* _pPrefab = LoadPrefab(a_context, m_platoonPrefabGUID, m_platoonPrefab);
		if (!_pPrefab)
		{
			ENGINE_WARNING("SwarmBossController : 小隊長生成用のプレハブが設定されていません");
			return false;
		}

		m_platoonLeaderEntities.reserve(m_maxPlatoonLeader);

		// 一列に並べるので、一つ前の相手とその位置を持ち回る
		Engine::ECS::Entity _preLeader = m_leaderEntity;
		Math::Vector3 _prePos = m_spawnPos;

		// 最大数分小隊長を生成
		for (uint32_t _i = 0; _i < m_maxPlatoonLeader; ++_i)
		{
			// プレハブのインスタンス取得
			std::vector<Engine::Resource::PrefabInstanceData> _instanceVec = {};
			if (!App::Utility::BuildSpawnInstanceData(_world, *_pPrefab, {}, _instanceVec))
			{
				ENGINE_ERRLOG(false, "SwarmBossController : 小隊長生成用のプレハブの取得に失敗");
				return false;
			}

			// 初期化時に一つ前の小隊長のIDをコンポーネントに覚えさせる
			// 初めの小隊長はリーダーのEntityIDを覚えさせる
			// 間隔はプレハブに保存された値を使う
			float _distance = 0.0f;
			EditRootComponent<PlatoonLeaderComponent>(_world, _instanceVec,
				[&](PlatoonLeaderComponent& a_comp)
				{
					a_comp.preLeader    = _preLeader;
					a_comp.platoonIndex = static_cast<int>(_i);
					_distance = a_comp.distance;
				});

			// 一つ前の相手の後ろへ間隔ぶん下げて置く
			const Math::Vector3 _pos = _prePos + PLATOON_LINE_DIR * _distance;
			EditRootComponent<LocalTransformComponent>(_world, _instanceVec,
				[&](LocalTransformComponent& a_comp)
				{
					a_comp.pos     = _pos;
					a_comp.isDirty = true;
				});

			// 必須なコンポーネントを付与 : すでにあればスキップ。
			// 速さはリーダーより速くしておく(同じだと離された分を詰められない)
			ApplyMoveSpeed(_world, _instanceVec, m_leaderSpeed * m_platoonSpeedScale);
			EnsureRootComponent<VelocityComponent>(_world, _instanceVec);

			// 前の相手の後ろを狙うのに前方が要る(LookAngle から作る)。
			// 上下も体ごと向く(列が潜っても機体の向きが進路と揃う)
			EditRootComponent<LookAngleComponent>(_world, _instanceVec,
				[](LookAngleComponent& a_comp)
				{
					a_comp.isApplyPitchToBody = true;
				}
			);

			const Engine::ECS::Entity _entity = App::Utility::CreateInstanceNow(_world, _instanceVec);
			if (_entity == Engine::ECS::Limits::INVALID_ENTITY)
			{
				ENGINE_ERRLOG(false, "SwarmBossController : 小隊長のエンティティを作れませんでした");
				return false;
			}

			m_platoonLeaderEntities.push_back(_entity);
			_preLeader = _entity;
			_prePos    = _pos;
		}

		// ボイド生成 : 小隊長が出揃ってから数を振り分ける
		bool _isSucceeded = true;
		for (size_t _i = 0; _i < m_platoonLeaderEntities.size(); ++_i)
		{
			const Engine::ECS::Entity _platoon = m_platoonLeaderEntities[_i];

			// 位置は生成時に書き込んだ値をそのまま読む(まだ行列は組まれていない)
			Math::Vector3 _center = m_spawnPos;
			if (_world.HasComponent<LocalTransformComponent>(_platoon))
			{
				_center = _world.RefData<LocalTransformComponent>(_platoon)->pos;
			}

			const uint32_t _count = GetBoidCountForPlatoon(_i, m_platoonLeaderEntities.size());
			if (!CreateBoids(a_context, _platoon, static_cast<int>(_i), _count, _center))
			{
				_isSucceeded = false;
			}
		}

		return _isSucceeded;
	}

	bool SwarmBossController::CreateBoids(
		Engine::GameObject::ObjectContext& a_context,
		Engine::ECS::Entity a_platoonLeader,
		int a_platoonIndex,
		uint32_t a_count,
		const Math::Vector3& a_center)
	{
		if (a_count == 0) return true;

		auto& _world = *a_context.pWorld;
		auto& _rm = *a_context.pServices->pResourceManager;

		// 出すボイドの設定は小隊長が持っている
		if (!_world.HasComponent<BoidSpownerComponent>(a_platoonLeader))
		{
			ENGINE_WARNING("SwarmBossController : 小隊長のプレハブに BoidSpownerComponent がありません");
			return false;
		}

		const Engine::Resource::Prefab* _pBoidPrefab = nullptr;
		float _radius = 0.0f;
		{
			// この後エンティティを作るとチャンクが動くことがあるので、
			// コンポーネントへのポインタはこのブロックの中だけで使う
			BoidSpownerComponent* _pSpowner = _world.RefData<BoidSpownerComponent>(a_platoonLeader);
			if (!_pSpowner->boidPrefabGUID.IsValid())
			{
				ENGINE_WARNING("SwarmBossController : BoidSpownerComponent のボイドプレハブが設定されていません");
				return false;
			}

			// 参照は小隊長のコンポーネントが持つ(返すのはその解放フック)。
			// 生成したばかりで入っている値は持ち主ではない(プレハブの保存値)ので、必ず1つ取る
			_rm.AcquireImmediate(_pSpowner->prefab, _pSpowner->boidPrefabGUID);

			_pBoidPrefab = _rm.Get(_pSpowner->prefab);
			_radius = _pSpowner->spawnRadius;
		}

		if (!_pBoidPrefab)
		{
			ENGINE_ERRLOG(false, "SwarmBossController : ボイド生成用のプレハブの取得に失敗");
			return false;
		}

		// ボイドの共通設定
		//   追従先 : 自分の小隊長(FollowLeaderSystem が目標地点へ流す)
		//   印     : このオブジェクトのGUID + 小隊番号(生存数を数える)
		App::Utility::SpawnParams _params = {};
		_params.followTarget     = a_platoonLeader;
		_params.followTargetGUID = GetEntityGUID(_world, a_platoonLeader);
		_params.spawnerGUID      = m_guid;
		_params.waveIndex        = a_platoonIndex;

		uint32_t _created = 0;
		for (uint32_t _i = 0; _i < a_count; ++_i)
		{
			_params.pos = a_center + RandomInSphere(_radius);

			std::vector<Engine::Resource::PrefabInstanceData> _instanceVec = {};
			if (!App::Utility::BuildSpawnInstanceData(_world, *_pBoidPrefab, _params, _instanceVec)) break;

			// 必須なコンポーネントを付与 : すでにあればスキップ。
			// 速さはリーダー・小隊長より速くしておく(最後尾なので一番速さが要る)。
			// 舵(maxSteeringForce)も速さに比例させないと、最高速に乗る前に曲がれなくなる
			const float _boidSpeed = m_leaderSpeed * m_boidSpeedScale;
			EditRootComponent<BoidComponent>(_world, _instanceVec,
				[&](BoidComponent& a_comp)
				{
					a_comp.platoonID        = a_platoonLeader;
					a_comp.maxSpeed         = _boidSpeed;
					a_comp.maxSteeringForce = _boidSpeed * 4.0f;
				}
			);
			ApplyMoveSpeed(_world, _instanceVec, _boidSpeed);
			EnsureRootComponent<VelocityComponent>(_world, _instanceVec);

			// ボスの体である印。Controller はこれを数えて体力にする
			EnsureRootComponent<SwarmBossBoidTag>(_world, _instanceVec);

			//--------------------------------------------------------------
			// 当たり判定
			//
			// 当たりに行く相手はボイド同士とプレイヤーの攻撃だけ。
			// 押し出し(isPhysical)は切ってある。ぶつかった分だけ離れるのは
			// ボイド側の反発(BoidComponent の separation)の仕事で、
			// そこへ押し出しを重ねると動きが硬くなる
			//--------------------------------------------------------------
			EditRootComponent<ColliderComponent>(_world, _instanceVec,
				[&](ColliderComponent& a_comp)
				{
					a_comp.layer        = Layer::SwarmBoid;
					a_comp.collideLayer = Layer::SwarmBoid | Layer::PlayerProjectile;
					a_comp.isPhysical   = 0;

					// Mesh 以外なので、ボディは描画メッシュのAABBの箱になる。
					// 半径は判定を出す側の SphereColliderComponent が持つ(下)
					a_comp.shapeType = Engine::Physics::EShapeType::Sphere;
				}
			);

			// 判定を出す側(HitDetectSystem)に要る球と、当たった結果の受け皿
			EditRootComponent<SphereColliderComponent>(_world, _instanceVec,
				[&](SphereColliderComponent& a_comp)
				{
					a_comp.radius = m_boidColliderRadius;
				}
			);
			EnsureRootComponent<Engine::ECS::CollisionEvent>(_world, _instanceVec);

			// 体力 : 落とされた1体ぶんがボスの体力1になる
			EditRootComponent<HealthComponent>(_world, _instanceVec,
				[&](HealthComponent& a_comp)
				{
					a_comp.maxHealth    = m_boidHealth;
					a_comp.releaseDelay = m_boidReleaseDelay;
				}
			);

			// 向きは所属している小隊長の向きへ寄せる(SwarmLookSystem)。上下も体ごと向く
			EditRootComponent<LookAngleComponent>(_world, _instanceVec,
				[](LookAngleComponent& a_comp)
				{
					a_comp.isApplyPitchToBody = true;
				}
			);

			if (App::Utility::CreateInstanceNow(_world, _instanceVec) == Engine::ECS::Limits::INVALID_ENTITY) break;
			++_created;
		}

		m_currentBoids += _created;

		if (_created != a_count)
		{
			ENGINE_WARNING("SwarmBossController : ボイドを出し切れませんでした(%u / %u)", _created, a_count);
			return false;
		}
		return true;
	}

	uint32_t SwarmBossController::GetBoidCountForPlatoon(size_t a_platoonIndex, size_t a_platoonCount) const
	{
		if (a_platoonCount == 0) return 0;

		const uint32_t _base = m_maxBoid / static_cast<uint32_t>(a_platoonCount);
		const uint32_t _rest = m_maxBoid % static_cast<uint32_t>(a_platoonCount);

		// 余りは前の小隊から1体ずつ足す(合計が最大数と一致するように)
		return _base + (a_platoonIndex < _rest ? 1u : 0u);
	}

	//======================================================================================
	// ボスの体力 : 自分が出したボイドの数
	//--------------------------------------------------------------------------------------
	// 印(SwarmBossBoidTag)を数えるだけ。ボイドのIDは持ち歩かないので、撃ち落とされて
	// 消えたぶんは次のフレームの数え上げで自然に減る。
	//
	// ・同じシーンに群れのボスが2体居ても混ざらないよう、生成元の印(SpawnerComponent)も見る。
	// ・死亡状態のものは数えない。体力が尽きてもすぐには消えず(演出の猶予)、
	//   ActiveTag はその間も付いたままなので、ここで外さないと落としたぶんが反映されない。
	//======================================================================================
	uint32_t SwarmBossController::CountAliveBoids(Engine::GameObject::ObjectContext& a_context) const
	{
		uint32_t _count = 0;
		const Engine::GUID _self = m_guid;

		// 解放待ち(ActiveTag が外れたもの)は数えない
		a_context.pWorld->ForEach<const ActiveTag, const SwarmBossBoidTag, const SpawnerComponent>(
			[&_count, &_self, &a_context](
				Engine::ECS::Chunk* a_pChunk,
				uint32_t a_count,
				const ActiveTag* a_activeTagArray,
				const SwarmBossBoidTag* a_boidTagArray,
				const SpawnerComponent* a_spawnerArray)
			{
				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					if (a_spawnerArray[_i].spawnerGUID != _self) continue;

					// 死亡状態(消えるのを待っているだけ)のものは体力に数えない
					const Engine::ECS::Entity _entity = a_pChunk->entityData[_i];
					if (a_context.pWorld->HasComponent<HealthComponent>(_entity))
					{
						const auto* _pHealth = a_context.pWorld->RefData<HealthComponent>(_entity);
						if (_pHealth && _pHealth->isDead) continue;
					}

					++_count;
				}
			}
		);

		return _count;
	}

	//======================================================================================
	// シリアライズ
	//======================================================================================
	void SwarmBossController::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		// ---- リーダー ----
		a_ar.GUIDField("LeaderPrefabGUID", m_leaderPrefabGUID);
		a_ar.Field("SpawnPos", m_spawnPos);

		// ---- 小隊長 ----
		a_ar.GUIDField("PlatoonPrefabGUID", m_platoonPrefabGUID);
		a_ar.Field("MaxPlatoonLeader", m_maxPlatoonLeader);

		// ---- ボイド ----
		a_ar.Field("MaxBoid", m_maxBoid);
		a_ar.Field("BoidColliderRadius", m_boidColliderRadius);
		a_ar.Field("BoidHealth", m_boidHealth);
		a_ar.Field("BoidReleaseDelay", m_boidReleaseDelay);

		// ---- 速さの配分 ----
		a_ar.Field("LeaderSpeed", m_leaderSpeed);
		a_ar.Field("PlatoonSpeedScale", m_platoonSpeedScale);
		a_ar.Field("BoidSpeedScale", m_boidSpeedScale);

		// ---- 行動 ----
		// 調整値は各ステートが持つ(名前は以前と同じなので既存シーンもそのまま読める)
		m_stateMachine.Archive(a_ar);
	}

	//======================================================================================
	// エディター
	//======================================================================================
	void SwarmBossController::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		if (!a_context.pServices) return;
		auto& _services = *a_context.pServices;

		ImGui::SeparatorText("Leader");
		Engine::Editor::EditorHelper::DrawAssetSelectComboGUID(_services, "Leader Prefab", "Prefab", m_leaderPrefabGUID);
		ImGui::DragFloat3("Spawn Pos", &m_spawnPos.x, 0.1f);

		ImGui::SeparatorText("Platoon Leader");
		Engine::Editor::EditorHelper::DrawAssetSelectComboGUID(_services, "Platoon Prefab", "Prefab", m_platoonPrefabGUID);
		ImGui::InputScalar("Max Platoon Leader", ImGuiDataType_U32, &m_maxPlatoonLeader);
		ImGui::TextDisabled("Line up behind the leader (-Z) by PlatoonLeaderComponent.distance");

		ImGui::SeparatorText("Boid");
		ImGui::InputScalar("Max Boid", ImGuiDataType_U32, &m_maxBoid);
		if (m_maxPlatoonLeader > 0)
		{
			ImGui::TextDisabled("Per platoon : %u (+1 for the first %u)",
				m_maxBoid / m_maxPlatoonLeader, m_maxBoid % m_maxPlatoonLeader);
		}
		ImGui::TextDisabled("Boid prefab / radius : BoidSpownerComponent on the platoon prefab");

		ImGui::DragFloat("Boid Collider Radius", &m_boidColliderRadius, 0.05f, 0.0f);
		ImGui::DragFloat("Boid Health", &m_boidHealth, 1.0f, 0.0f);
		ImGui::DragFloat("Boid Release Delay", &m_boidReleaseDelay, 0.05f, 0.0f);
		ImGui::TextDisabled("Hit : boid vs boid / player attacks only");

		ImGui::SeparatorText("Speed");
		ImGui::DragFloat("Leader Speed", &m_leaderSpeed, 0.5f, 0.0f);
		ImGui::DragFloat("Platoon Scale", &m_platoonSpeedScale, 0.05f, 0.0f);
		ImGui::DragFloat("Boid Scale", &m_boidSpeedScale, 0.05f, 0.0f);
		ImGui::TextDisabled("Platoon %.1f / Boid %.1f (written on spawn, overrides prefab)",
			m_leaderSpeed * m_platoonSpeedScale, m_leaderSpeed * m_boidSpeedScale);

		ImGui::SeparatorText("Leader Action");
		m_stateMachine.DrawInspector();

		// ここから下は実行中の状態なので表示のみ
		ImGui::SeparatorText("Runtime");
		ImGui::Text("Spawned : %s", m_isSpown ? "yes" : "no");
		if (!m_isSpown)
		{
			// 置いた直後はプレハブ未設定のまま Awake を通っているので、設定してから出せるようにする
			ImGui::SameLine();
			if (Engine::Editor::EditorHelper::CreateSmallButton("Spawn"))
			{
				Spawn(a_context);
			}
		}

		ImGui::Text("Leader  : %llu", static_cast<unsigned long long>(m_leaderEntity));
		ImGui::Text("Platoon : %u / %u", static_cast<uint32_t>(m_platoonLeaderEntities.size()), m_maxPlatoonLeader);
		for (size_t _i = 0; _i < m_platoonLeaderEntities.size(); ++_i)
		{
			ImGui::BulletText("[%u] %llu", static_cast<uint32_t>(_i),
				static_cast<unsigned long long>(m_platoonLeaderEntities[_i]));
		}
		ImGui::Text("HP      : %u / %u (alive boids)", m_currentBoids, m_maxBoid);
	}
}
