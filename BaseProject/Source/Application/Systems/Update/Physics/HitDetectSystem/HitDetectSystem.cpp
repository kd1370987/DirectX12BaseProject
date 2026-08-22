#include "HitDetectSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/ECS/Internal/CollisionEvent.h"

#include "Engine/MainEngine.h"
#include "Engine/Collision/CollisionWorld.h"

#include "Application/Components/Collision/SphereCollider.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Character/Weapon/Projectile/ProjectileComponent.h"
#include "Application/InstanceResource/HitEventResource.h"

void HitDetectSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const SphereColliderComponent, Engine::ECS::CollisionEvent, const LocalTransformComponent>(
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
			const LocalTransformComponent* a_transArray
			)
		{
			auto* _pCollWorld = &a_ctx.pWorld->GetResource<Engine::Collision::CollisionWorld>();

			// ワールド側のヒット履歴(反応系が横から読む)
			HitEventResource* _pHitEvents = nullptr;
			if (a_ctx.pWorld->HasResource<HitEventResource>())
			{
				_pHitEvents = &a_ctx.pWorld->GetResource<HitEventResource>();
			}

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const SphereColliderComponent& _sphere = a_sphereArray[_i];
				const LocalTransformComponent& _trans = a_transArray[_i];
				Engine::ECS::Entity _self = a_pChunk->entityData[_i];

				// 投射物なら、与えるダメージと発射元を拾っておく。
				// 銃口は撃った本人の体の中にあるので、除外しないと発射した瞬間に
				// 自分へ当たって消える。ProjectileComponent は弾しか持たないので、
				// クエリには含めず持っている時だけ引く。
				Engine::ECS::Entity _shooter = Engine::ECS::Limits::INVALID_ENTITY;
				float               _damage  = 0.0f;
				ProjectileComponent* _pProjectile = nullptr;
				if (a_ctx.pWorld->HasComponent<ProjectileComponent>(_self))
				{
					_pProjectile = a_ctx.pWorld->RefData<ProjectileComponent>(_self);
					if (_pProjectile)
					{
						_shooter = _pProjectile->shooterEntity;
						_damage  = _pProjectile->damage;
					}
				}

				const Math::Vector3 _nowPos = Math::Vector3(_trans.pos) + Math::Vector3(_sphere.offset);

				//==========================================================
				// 判定
				//----------------------------------------------------------
				// 弾のように速いものは「今いる場所の球」だけで見てはいけない。
				// 1フレームの移動量が球の直径を超えると、その間はどこも判定されず
				// 相手を飛び越してしまう(弾速100m/s・60fpsなら1フレーム約1.7m進むのに
				// 判定球は直径0.3m。間の約1.4mが素通りになる)。
				// フレームレートが落ちるほど移動量が伸びるので、重い場面ほどよく抜ける。
				//
				// そこで前フレームの位置と今の位置を結んだカプセル(=球を掃いた形)で見る。
				// 動いていない/移動量が小さいものは今までどおり球で判定する。
				//==========================================================
				Engine::Collision::Result _res = {};
				bool _isHit = false;

				bool _isSwept = false;
				if (_pProjectile && _pProjectile->hasPrevPos)
				{
					const Math::Vector3 _prevPos = Math::Vector3(_pProjectile->prevPos);
					const Math::Vector3 _move    = _nowPos - _prevPos;

					// 球の半径ぶんも動いていないなら、球のままで取りこぼさない
					if (_move.LengthSquared() > (_sphere.radius * _sphere.radius))
					{
						Engine::Collision::CapsuleInfo _capsuleInfo;
						_capsuleInfo.pointA = _prevPos;
						_capsuleInfo.pointB = _nowPos;
						_capsuleInfo.radius = _sphere.radius;

						_isHit   = _pCollWorld->VsCapsule(_capsuleInfo, _res, _self, _shooter);
						_isSwept = true;
					}
				}

				if (!_isSwept)
				{
					// 自分の球で重なりクエリ(自分自身と発射元は除外)
					Engine::Collision::SphereInfo _info;
					_info.origin = _nowPos;
					_info.radius = _sphere.radius;

					_isHit = _pCollWorld->VsSphere(_info, _res, _self, _shooter);
				}

				// 次フレームの判定の起点にする。当たったかどうかに関わらず更新する
				if (_pProjectile)
				{
					_pProjectile->prevPos    = _nowPos;
					_pProjectile->hasPrevPos = true;
				}

				if (!_isHit) continue;
				if (!_res.isHit) continue;

				// 自分側に記録(弾が hitPos で反応/消滅するため)
				a_eventArray[_i].other  = _res.hitEntity;
				a_eventArray[_i].hitPos = _res.hitPos;
				a_eventArray[_i].hitDir = _res.hitNormal;

				// 当たった相手側にも直接書き込む(相手が CollisionEvent を持っていれば)。
				// 値の書き換えのみ＝構造変化なしなので反復中でも安全。
				if (_res.hitEntity != Engine::ECS::Limits::INVALID_ENTITY &&
					a_ctx.pWorld->HasComponent<Engine::ECS::CollisionEvent>(_res.hitEntity))
				{
					auto* _ev = a_ctx.pWorld->RefData<Engine::ECS::CollisionEvent>(_res.hitEntity);
					if (_ev)
					{
						_ev->other  = _self;
						_ev->hitPos = _res.hitPos;
						// 相手から見た方向は逆
						_ev->hitDir = { -_res.hitNormal.x, -_res.hitNormal.y, -_res.hitNormal.z };
					}
				}

				// ワールドのヒットイベントにも1件積む。
				// CollisionEvent は1エンティティ1件しか持てず、消えた弾の分も残らないので、
				// エフェクト生成やのけぞりなどの反応系はこちらを読む。
				if (_pHitEvents && _res.hitEntity != Engine::ECS::Limits::INVALID_ENTITY)
				{
					HitEvent _hit = {};
					_hit.attacker = _self;
					// 弾を撃った本体。ヒットマーカーのように「自分の弾が当たったか」を
					// 見たい側は、弾(attacker)ではなくこちらで判定する
					_hit.shooter  = _shooter;
					_hit.victim   = _res.hitEntity;
					_hit.hitPos   = _res.hitPos;
					// 受けた側の体力を削る量(HealthSystem が読む)。弾以外は 0 のまま
					_hit.damage   = _damage;
					// 受けた側から見た方向にそろえる(のけぞりの向きに使う)
					_hit.hitDir   = { -_res.hitNormal.x, -_res.hitNormal.y, -_res.hitNormal.z };
					// このタスクを通るのは球コライダー＋CollisionEvent を持つ弾だけなので Bullet 固定。
					// 近接など別経路が増えたら、産む側で種別を指定すること。
					_hit.type     = EHitEventType::Bullet;

					_pHitEvents->Push(_hit);
				}
			}
		}
	);
}
