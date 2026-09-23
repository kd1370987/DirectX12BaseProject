#include "BoidContactDamageSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "../../../../Components/Character/Boss/BoidContactDamageComponent.h"
#include "../../../../Components/Character/HealthComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../InstanceResource/SwarmContactDamageResource.h"
#include "../../../../InstanceResource/HitEventResource.h"

//==============================================================================
// BoidContactDamageSystem
//
// ワームの体(ボイド)がプレイヤーに触れたら、ボイド1体ぶんのダメージを与える。
// 与えたボイドは cooldown 秒のあいだ判定しない(ボイドごと)。
//
//   接触 = ボイドの中心からプレイヤーのカプセルの線分までの距離 < ボイドの半径 + カプセルの半径
//
// ・物理のクエリは使わない。相手はプレイヤー1体だけなので、距離の計算で足りる
//   (4000 体ぶんクエリを打つより桁違いに軽い)。
// ・ダメージは HitEventResource に積むだけ。体力を減らすのは HealthSystem(PostUpdate)、
//   被弾の反応(音・のけぞりなど)も同じヒットを読む。積むのは PreUpdate のクリアより後、
//   HealthSystem より前でなければならないので Update 帯に置いている。
// ・死亡状態のボイド(撃ち落とされて消えるのを待っているもの)は当たらない。
// ・プレイヤーの形は SwarmBossController が毎フレーム SwarmContactDamageResource に書く。
//==============================================================================
namespace
{
	// 線分 AB 上で点 P にいちばん近い点
	Math::Vector3 ClosestPointOnSegment(const Math::Vector3& a_p, const Math::Vector3& a_a, const Math::Vector3& a_b)
	{
		const Math::Vector3 _ab = a_b - a_a;
		const float _lenSq = _ab.LengthSquared();
		if (_lenSq <= 1e-8f) return a_a;

		const float _t = std::clamp((a_p - a_a).Dot(_ab) / _lenSq, 0.0f, 1.0f);
		return a_a + _ab * _t;
	}
}

void BoidContactDamageSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<BoidContactDamageComponent, const LocalTransformComponent, const HealthComponent>(
		Engine::ECS::ESystemType::Update,
		"BoidContactDamageSystem",
		[](
			Engine::ECS::Chunk*               a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			BoidContactDamageComponent*       a_contactArray,
			const LocalTransformComponent*    a_transArray,
			const HealthComponent*            a_healthArray
		)
		{
			auto& _world = *a_ctx.pWorld;
			if (!_world.HasResource<SwarmContactDamageResource>()) return;
			if (!_world.HasResource<HitEventResource>()) return;

			const auto& _res = _world.GetResource<SwarmContactDamageResource>();
			auto& _hitEvents = _world.GetResource<HitEventResource>();

			const float _dt = a_ctx.dt;
			const bool _isActive = _res.isActive && _res.player != Engine::ECS::Limits::INVALID_ENTITY;
			const float _hitDistance   = _res.boidRadius + _res.playerRadius;
			const float _hitDistanceSq = _hitDistance * _hitDistance;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				BoidContactDamageComponent& _contact = a_contactArray[_i];

				// 待ち時間はプレイヤーが居なくても進める(居ない間に止まったままにならないように)
				if (_contact.cooldownTimer > 0.0f)
				{
					_contact.cooldownTimer -= _dt;
					continue;
				}

				if (!_isActive) continue;
				if (a_healthArray[_i].isDead) continue;

				const Math::Vector3 _boidPos = a_transArray[_i].pos;
				const Math::Vector3 _closest =
					ClosestPointOnSegment(_boidPos, _res.playerSegmentA, _res.playerSegmentB);

				const Math::Vector3 _toPlayer = _closest - _boidPos;
				const float _distSq = _toPlayer.LengthSquared();
				if (_distSq >= _hitDistanceSq) continue;

				// 受けた向き(プレイヤーから見て押される向き) = ボイドからプレイヤーへ
				Math::Vector3 _dir = _toPlayer;
				if (_distSq > 1e-8f) _dir /= std::sqrt(_distSq);
				else _dir = Math::Vector3(0.0f, 1.0f, 0.0f);

				HitEvent _event = {};
				_event.attacker = a_pChunk->entityData[_i];
				_event.victim   = _res.player;
				_event.hitPos   = _closest - _dir * _res.playerRadius;	// カプセルの表面
				_event.hitDir   = _dir;
				_event.damage   = _res.damage;
				_event.type     = EHitEventType::Contact;
				_hitEvents.Push(_event);

				_contact.cooldownTimer = _res.cooldown;
			}
		}
	);
}
