#include "HitDetectSystem.h"

#include "Application/ECS/World/APPWorld.h"
#include "Engine/ECS/Internal/CollisionEvent.h"

#include "Engine/MainEngine.h"
#include "Engine/Collision/CollisionWorld.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Option/OptionManager.h"

#include "Application/Components/Collision/SphereCollider.h"
#include "Application/Components/Collision/Collider.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Character/Weapon/Projectile/ProjectileComponent.h"
#include "Application/InstanceResource/HitEventResource.h"

#include "../../../Shared/PhysicsCompare/PhysicsCompare.h"

namespace
{
	// 1発ぶんの判定。旧と Jolt の結果を並べて持つ
	struct HitProbe
	{
		// ---- 入力 ----
		Engine::ECS::Entity self = Engine::ECS::Limits::INVALID_ENTITY;
		Engine::ECS::Entity shooter = Engine::ECS::Limits::INVALID_ENTITY;
		ProjectileComponent* pProjectile = nullptr;
		float damage = 0.0f;
		float radius = 0.0f;
		uint32_t layerMask = 0;

		Math::Vector3 nowPos = {};
		Math::Vector3 prevPos = {};
		bool isSwept = false;		// 前フレームの位置から掃くか(false なら今の位置の球だけ)

		// ---- 旧(CollisionWorld) : 静的→動的の順に、最初に触れた1体 ----
		bool oldHit = false;
		Engine::ECS::Entity oldEntity = Engine::ECS::Limits::INVALID_ENTITY;
		Math::Vector3 oldPos = {};
		Math::Vector3 oldNormal = {};	// 旧は常にゼロ

		// ---- Jolt(PhysicsWorld) : 進む向きでいちばん手前 / 重なりはいちばん深い1体 ----
		bool joltHit = false;
		Engine::ECS::Entity joltEntity = Engine::ECS::Limits::INVALID_ENTITY;
		Math::Vector3 joltPos = {};
		Math::Vector3 joltNormal = {};
	};
}

void HitDetectSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<const SphereColliderComponent, Engine::ECS::CollisionEvent, const LocalTransformComponent, const ColliderComponent>(
		Engine::ECS::ESystemType::Physics,
		"HitDetectSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const SphereColliderComponent* a_sphereArray,
			Engine::ECS::CollisionEvent* a_eventArray,
			const LocalTransformComponent* a_transArray,
			const ColliderComponent* a_collArray
			)
		{
			// 旧(CollisionWorld)/ Jolt(PhysicsWorld)のどちらで判定するか。移行中だけの切り替え
			const auto& _migration = a_ctx.pServices->pOptionManager->GetPhysicsMigrationOption();

			// ワールド側のヒット履歴(反応系が横から読む)
			HitEventResource* _pHitEvents = nullptr;
			if (a_ctx.pWorld->HasResource<HitEventResource>())
			{
				_pHitEvents = &a_ctx.pWorld->GetResource<HitEventResource>();
			}

			// ECS はシングルスレッドなので置き場は使い回す
			static std::vector<HitProbe> s_probes;
			s_probes.assign(a_count, {});

			//==========================================================
			// 入力をそろえる
			//==========================================================
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				HitProbe& _probe = s_probes[_i];
				const SphereColliderComponent& _sphere = a_sphereArray[_i];
				const LocalTransformComponent& _trans = a_transArray[_i];

				// 当たりに行くレイヤー。
				// 弾は発射のたびに ProjectileSpawn が撃った側で入れ替えているので、
				// 自分側の弾のレイヤーはここに入っていない(自分の弾同士がすり抜ける)
				_probe.layerMask = static_cast<uint32_t>(a_collArray[_i].collideLayer);

				_probe.self = a_pChunk->entityData[_i];
				_probe.radius = _sphere.radius;

				// 投射物なら、与えるダメージと発射元を拾っておく。
				// 銃口は撃った本人の体の中にあるので、除外しないと発射した瞬間に
				// 自分へ当たって消える。ProjectileComponent は弾しか持たないので、
				// クエリには含めず持っている時だけ引く。
				if (a_ctx.pWorld->HasComponent<ProjectileComponent>(_probe.self))
				{
					_probe.pProjectile = a_ctx.pWorld->RefData<ProjectileComponent>(_probe.self);
					if (_probe.pProjectile)
					{
						_probe.shooter = _probe.pProjectile->shooterEntity;
						_probe.damage  = _probe.pProjectile->damage;
					}
				}

				_probe.nowPos = Math::Vector3(_trans.pos) + Math::Vector3(_sphere.offset);

				//==========================================================
				// 弾のように速いものは「今いる場所の球」だけで見てはいけない。
				// 1フレームの移動量が球の直径を超えると、その間はどこも判定されず
				// 相手を飛び越してしまう(弾速100m/s・60fpsなら1フレーム約1.7m進むのに
				// 判定球は直径0.3m。間の約1.4mが素通りになる)。
				// フレームレートが落ちるほど移動量が伸びるので、重い場面ほどよく抜ける。
				//
				// そこで前フレームの位置から今の位置まで球を掃いて見る。
				// 動いていない/移動量が小さいものは今までどおり球で判定する。
				//==========================================================
				if (_probe.pProjectile && _probe.pProjectile->hasPrevPos)
				{
					_probe.prevPos = Math::Vector3(_probe.pProjectile->prevPos);
					const Math::Vector3 _move = _probe.nowPos - _probe.prevPos;

					// 球の半径ぶんも動いていないなら、球のままで取りこぼさない
					_probe.isSwept = _move.LengthSquared() > (_probe.radius * _probe.radius);
				}
			}

			//==========================================================
			// 旧 : 前→今を結んだカプセル(=球を掃いた形)/ 今の球 で重なりを見る。
			// 静的→動的の順に探し、最初に触れた1体で確定する
			//==========================================================
			if (_migration.RunsOldHit())
			{
				ENGINE_PROFILE_SCOPE("Collision_HitDetect");
				auto& _collWorld = a_ctx.pWorld->GetResource<Engine::Collision::CollisionWorld>();
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					HitProbe& _probe = s_probes[_i];
					Engine::Collision::Result _res = {};
					bool _isHit = false;

					if (_probe.isSwept)
					{
						Engine::Collision::CapsuleInfo _capsuleInfo;
						_capsuleInfo.pointA = _probe.prevPos;
						_capsuleInfo.pointB = _probe.nowPos;
						_capsuleInfo.radius = _probe.radius;
						_isHit = _collWorld.VsCapsule(_capsuleInfo, _res, _probe.self, _probe.shooter, _probe.layerMask);
					}
					else
					{
						Engine::Collision::SphereInfo _info;
						_info.origin = _probe.nowPos;
						_info.radius = _probe.radius;
						_isHit = _collWorld.VsSphere(_info, _res, _probe.self, _probe.shooter, _probe.layerMask);
					}

					_probe.oldHit = _isHit && _res.isHit;
					_probe.oldEntity = _res.hitEntity;
					_probe.oldPos = _res.hitPos;
					_probe.oldNormal = _res.hitNormal;
				}
			}

			//==========================================================
			// Jolt : 前→今へ球を掃いて、進む向きでいちばん手前 / 今の球でいちばん深い1体
			//==========================================================
			if (_migration.RunsJoltHit())
			{
				ENGINE_PROFILE_SCOPE("Physics_HitDetect");
				const auto& _physicsWorld = a_ctx.pWorld->GetResource<Engine::Physics::PhysicsWorld>();
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					HitProbe& _probe = s_probes[_i];
					Engine::Physics::ShapeHit _hit = {};

					_probe.joltHit = _probe.isSwept
						? _physicsWorld.SweepSphere(_probe.prevPos, _probe.nowPos, _probe.radius,
							_probe.layerMask, _probe.self, _probe.shooter, _hit)
						: _physicsWorld.OverlapSphere(_probe.nowPos, _probe.radius,
							_probe.layerMask, _probe.self, _probe.shooter, _hit);

					_probe.joltEntity = _hit.entity;
					_probe.joltPos = _hit.position;
					_probe.joltNormal = _hit.normal;
				}
			}

			//==========================================================
			// 比較
			//----------------------------------------------------------
			// 当たった/外れたの食い違いだけを「ずれ」にする。
			// 両方当たって相手が違うのは、旧が「最初に見つけた1体(静的が先)」、
			// Jolt が「進む向きでいちばん手前」なので起こりうる(群れを抜ける弾など)。別に数える。
			// 当たった位置も、旧は三角形の点/AABBの中心、Jolt は表面の接触点なので比べない
			//==========================================================
			if (_migration.compareQueries)
			{
				static App::Systems::PhysicsCompare::Stats s_stats{ "HitDetect" };
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const HitProbe& _probe = s_probes[_i];
					++s_stats.queries;

					if (_probe.oldHit == _probe.joltHit)
					{
						if (_probe.oldHit && _probe.oldEntity != _probe.joltEntity) ++s_stats.differentTarget;
						continue;
					}

					++s_stats.mismatches;
					if (App::Systems::PhysicsCompare::ShouldLogDetail(s_stats))
					{
						ENGINE_LOG("[PhysicsCompare] HitDetect mismatch self=%llu swept=%d from=(%.3f,%.3f,%.3f) to=(%.3f,%.3f,%.3f) r=%.3f mask=0x%x old=%d->%llu jolt=%d->%llu",
							_probe.self, _probe.isSwept ? 1 : 0,
							_probe.prevPos.x, _probe.prevPos.y, _probe.prevPos.z,
							_probe.nowPos.x, _probe.nowPos.y, _probe.nowPos.z,
							_probe.radius, _probe.layerMask,
							_probe.oldHit ? 1 : 0, _probe.oldEntity,
							_probe.joltHit ? 1 : 0, _probe.joltEntity);
					}
				}
				App::Systems::PhysicsCompare::Report(s_stats);
			}

			//==========================================================
			// 結果を反映
			//==========================================================
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const HitProbe& _probe = s_probes[_i];

				// 次フレームの判定の起点にする。当たったかどうかに関わらず更新する
				if (_probe.pProjectile)
				{
					_probe.pProjectile->prevPos    = _probe.nowPos;
					_probe.pProjectile->hasPrevPos = true;
				}

				const bool _useJolt = _migration.useJoltHitQueries;
				const bool _isHit = _useJolt ? _probe.joltHit : _probe.oldHit;
				if (!_isHit) continue;

				const Engine::ECS::Entity _hitEntity = _useJolt ? _probe.joltEntity : _probe.oldEntity;
				const Math::Vector3& _hitPos = _useJolt ? _probe.joltPos : _probe.oldPos;

				// 相手の表面の法線(こちらを向く)。旧は常にゼロ
				const Math::Vector3& _hitNormal = _useJolt ? _probe.joltNormal : _probe.oldNormal;

				// 自分側に記録(弾が hitPos で反応/消滅するため)
				a_eventArray[_i].other  = _hitEntity;
				a_eventArray[_i].hitPos = _hitPos;
				a_eventArray[_i].hitDir = _hitNormal;

				// 当たった相手側にも直接書き込む(相手が CollisionEvent を持っていれば)。
				// 値の書き換えのみ＝構造変化なしなので反復中でも安全。
				if (_hitEntity != Engine::ECS::Limits::INVALID_ENTITY &&
					a_ctx.pWorld->HasComponent<Engine::ECS::CollisionEvent>(_hitEntity))
				{
					auto* _ev = a_ctx.pWorld->RefData<Engine::ECS::CollisionEvent>(_hitEntity);
					if (_ev)
					{
						_ev->other  = _probe.self;
						_ev->hitPos = _hitPos;
						// 相手から見た方向は逆
						_ev->hitDir = -_hitNormal;
					}
				}

				// ワールドのヒットイベントにも1件積む。
				// CollisionEvent は1エンティティ1件しか持てず、消えた弾の分も残らないので、
				// エフェクト生成やのけぞりなどの反応系はこちらを読む。
				if (_pHitEvents && _hitEntity != Engine::ECS::Limits::INVALID_ENTITY)
				{
					HitEvent _hit = {};
					_hit.attacker = _probe.self;
					// 弾を撃った本体。ヒットマーカーのように「自分の弾が当たったか」を
					// 見たい側は、弾(attacker)ではなくこちらで判定する
					_hit.shooter  = _probe.shooter;
					_hit.victim   = _hitEntity;
					_hit.hitPos   = _hitPos;
					// 受けた側の体力を削る量(HealthSystem が読む)。弾以外は 0 のまま
					_hit.damage   = _probe.damage;
					// 受けた側から見た方向にそろえる(のけぞりの向きに使う)
					_hit.hitDir   = -_hitNormal;
					// このタスクを通るのは球コライダー＋CollisionEvent を持つ弾だけなので Bullet 固定。
					// 近接など別経路が増えたら、産む側で種別を指定すること。
					_hit.type     = EHitEventType::Bullet;

					_pHitEvents->Push(_hit);
				}
			}
		}
	);
}
