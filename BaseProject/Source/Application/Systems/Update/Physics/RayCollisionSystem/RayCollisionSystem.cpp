#include "RayCollisionSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Collision/Collider.h"
#include "Application/Components/Collision/RayCollider.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"
#include "../../../../Components/Resource/StateMachineComponent.h"

#include "../../../Shared/PhysicsCompare/PhysicsCompare.h"

#include "Engine/MainEngine.h"
#include "Engine/Collision/CollisionWorld.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Option/OptionManager.h"
#include "Engine/Graphics/DebugDraw/DebugDraw.h"
#include "Engine/Common/Color.h"

namespace
{
	// 1体ぶんの接地レイ。旧と Jolt の結果を並べて持つ
	struct GroundProbe
	{
		Engine::Collision::RayInfo ray = {};

		bool oldHit = false;
		Math::Vector3 oldPos = {};
		Engine::ECS::Entity oldEntity = Engine::ECS::Limits::INVALID_ENTITY;

		bool joltHit = false;
		Math::Vector3 joltPos = {};
	};
}

void RayCollisionSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<const ColliderComponent, const RayColliderComponent, LocalTransformComponent, VelocityComponent, StateMachineComponent>(
		Engine::ECS::ESystemType::Physics,
		"RayCollisionSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_activeTag,
			const ColliderComponent* a_collArray,
			const RayColliderComponent* a_rayArray,
			LocalTransformComponent* a_transArray,
			VelocityComponent* a_velArray,
			StateMachineComponent* a_stateArray
			)
		{
			// 旧(CollisionWorld)/ Jolt(PhysicsWorld)のどちらで判定するか。移行中だけの切り替え
			const auto& _migration = a_ctx.pServices->pOptionManager->GetPhysicsMigrationOption();

			// ECS はシングルスレッドなので置き場は使い回す
			static std::vector<GroundProbe> s_probes;
			s_probes.assign(a_count, {});

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				// 足元原点。「段差許容分だけ上」を発射点にして真下へ1本撃つ。
				// カバー範囲 : [足元 - snapDown, 足元 + stepUp]
				const LocalTransformComponent& _trans = a_transArray[_i];
				const RayColliderComponent& _ray = a_rayArray[_i];

				Engine::Collision::RayInfo& _info = s_probes[_i].ray;
				_info.origin = _trans.pos;
				_info.origin.y += _ray.stepUp;					// 段差分だけ上げる（浮かしも兼ねる）
				_info.direction = { 0.0f, -1.0f, 0.0f };
				_info.maxDistance = _ray.stepUp + _ray.snapDown;	// 上下をまとめて1本
			}

			// ---- 旧 : 静的・動的の両方のツリーを見る ----
			if (_migration.RunsOld())
			{
				ENGINE_PROFILE_SCOPE("Collision_GroundRay");
				auto& _collWorld = a_ctx.pWorld->GetResource<Engine::Collision::CollisionWorld>();
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					GroundProbe& _probe = s_probes[_i];
					Engine::Collision::Result _res = {};
					_probe.oldHit = _collWorld.Raycast(_probe.ray, _res, a_pChunk->entityData[_i]);
					_probe.oldPos = _res.hitPos;
					_probe.oldEntity = _res.hitEntity;
				}
			}

			// ---- Jolt : 今は静的なボディだけ(動くものは Phase 4 で入る)。レイヤーは旧と同じく全部 ----
			if (_migration.RunsJolt())
			{
				ENGINE_PROFILE_SCOPE("Physics_GroundRay");
				const auto& _physicsWorld = a_ctx.pWorld->GetResource<Engine::Physics::PhysicsWorld>();
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					GroundProbe& _probe = s_probes[_i];
					Engine::Physics::RayHit _hit = {};
					_probe.joltHit = _physicsWorld.CastRay(
						_probe.ray, Engine::Physics::kQueryAllLayers, a_pChunk->entityData[_i], _hit);
					_probe.joltPos = _hit.position;
				}
			}

			// ---- 比較 ----
			if (_migration.compareQueries)
			{
				static App::Systems::PhysicsCompare::Stats s_stats{ "GroundRay" };
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const GroundProbe& _probe = s_probes[_i];
					++s_stats.queries;

					const bool _isSame = (_probe.oldHit == _probe.joltHit) &&
						(!_probe.oldHit || App::Systems::PhysicsCompare::IsClose(_probe.oldPos, _probe.joltPos, _migration.compareTolerance));
					if (_isSame) continue;

					++s_stats.mismatches;

					// 旧が動くもの(敵・弾)に当たったぶんは、Jolt 側にまだ居ないのでずれて当然
					bool _isExpected = false;
					if (_probe.oldHit && _probe.oldEntity != Engine::ECS::Limits::INVALID_ENTITY &&
						a_ctx.pWorld->HasComponent<ColliderComponent>(_probe.oldEntity))
					{
						const auto* _pColl = a_ctx.pWorld->RefData<ColliderComponent>(_probe.oldEntity);
						_isExpected = _pColl && IsDynamicLayer(_pColl->layer);
					}
					if (_isExpected)
					{
						++s_stats.expected;
						continue;
					}

					if (App::Systems::PhysicsCompare::ShouldLogDetail(s_stats))
					{
						ENGINE_LOG("[PhysicsCompare] GroundRay mismatch entity=%llu origin=(%.3f,%.3f,%.3f) old=%d(%.4f,%.4f,%.4f) jolt=%d(%.4f,%.4f,%.4f)",
							a_pChunk->entityData[_i],
							_probe.ray.origin.x, _probe.ray.origin.y, _probe.ray.origin.z,
							_probe.oldHit ? 1 : 0, _probe.oldPos.x, _probe.oldPos.y, _probe.oldPos.z,
							_probe.joltHit ? 1 : 0, _probe.joltPos.x, _probe.joltPos.y, _probe.joltPos.z);
					}
				}
				App::Systems::PhysicsCompare::Report(s_stats);
			}

			// ---- 結果を反映 ----
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				LocalTransformComponent& _trans = a_transArray[_i];
				VelocityComponent& _vel = a_velArray[_i];
				StateMachineComponent& _state = a_stateArray[_i];
				const GroundProbe& _probe = s_probes[_i];

				const bool _isHit = _migration.useJoltStaticQueries ? _probe.joltHit : _probe.oldHit;
				const Math::Vector3& _hitPos = _migration.useJoltStaticQueries ? _probe.joltPos : _probe.oldPos;

				// プローブのデバッグ表示（緑=接地, 赤=空中。終点に球）
				a_ctx.pServices->pDebugDraw->DrawRay(
					_probe.ray.origin, _probe.ray.direction, _probe.ray.maxDistance, _isHit,
					_isHit ? Engine::Color::GREEN : Engine::Color::RED);

				// 範囲内に地面が無い → 空中
				if (!_isHit)
				{
					_state.isGround = false;
					continue;
				}

				// ジャンプ上昇中はスナップしない（頭上の段差に吸い付かないように）
				if (_vel.value.y > 0.0f)
				{
					_state.isGround = false;
					continue;
				}

				// 足元を地面へスナップ（段差登り／下り吸着 の両方をこれ1つで表現）
				_trans.pos.y = _hitPos.y;
				_trans.isDirty = true;
				_vel.value.y = 0.0f;
				_state.isGround = true;
			}
		}
	);
}
