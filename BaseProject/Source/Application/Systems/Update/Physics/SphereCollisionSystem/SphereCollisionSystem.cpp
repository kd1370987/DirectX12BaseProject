#include "SphereCollisionSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Collision/SphereCollider.h"
#include "Application/Components/Transform/LocalTransformComponent.h"

#include "../../../Shared/PhysicsCompare/PhysicsCompare.h"

#include "Engine/MainEngine.h"
#include "Engine/Collision/CollisionWorld.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Option/OptionManager.h"
#include "Engine/Graphics/DebugDraw/DebugDraw.h"
#include "Engine/Common/Color.h"

namespace
{
	// 1体ぶんの押し出し。旧と Jolt の結果を並べて持つ
	struct SphereResolve
	{
		Math::Vector3 center = {};

		bool oldHit = false;
		Math::Vector3 oldCorrection = {};

		bool joltHit = false;
		Math::Vector3 joltCorrection = {};
	};
}

void SphereCollisionSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<const SphereColliderComponent, LocalTransformComponent>(
		Engine::ECS::ESystemType::Physics,
		"SphereCollisionSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_activeTag,
			const SphereColliderComponent* a_sphereArray,
			LocalTransformComponent* a_transArray
			)
		{
			// 旧(CollisionWorld)/ Jolt(PhysicsWorld)のどちらで判定するか。移行中だけの切り替え
			const auto& _migration = a_ctx.pServices->pOptionManager->GetPhysicsMigrationOption();

			// ECS はシングルスレッドなので置き場は使い回す(ボイドは1チャンクに数百体来る)
			static std::vector<SphereResolve> s_resolves;
			s_resolves.assign(a_count, {});

			// 中心
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				s_resolves[_i].center = Math::Vector3(a_transArray[_i].pos) + Math::Vector3(a_sphereArray[_i].offset);
			}

			// ---- 旧 : 静的・動的の両方のツリーのメッシュから押し出す ----
			if (_migration.RunsOld())
			{
				ENGINE_PROFILE_SCOPE("Collision_ResolveSphere");
				auto& _collWorld = a_ctx.pWorld->GetResource<Engine::Collision::CollisionWorld>();
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					SphereResolve& _resolve = s_resolves[_i];
					Math::Vector3 _center = _resolve.center;
					_resolve.oldHit = _collWorld.ResolveSphere(
						_center, a_sphereArray[_i].radius, a_pChunk->entityData[_i], _resolve.oldCorrection, 4);
				}
			}

			// ---- Jolt : 今は静的なボディだけ(動くものは Phase 4 で入る)。レイヤーは旧と同じく全部 ----
			if (_migration.RunsJolt())
			{
				ENGINE_PROFILE_SCOPE("Physics_ResolveSphere");
				const auto& _physicsWorld = a_ctx.pWorld->GetResource<Engine::Physics::PhysicsWorld>();
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					SphereResolve& _resolve = s_resolves[_i];
					Math::Vector3 _center = _resolve.center;
					_resolve.joltHit = _physicsWorld.ResolveSphere(
						_center, a_sphereArray[_i].radius,
						Engine::Physics::kQueryAllLayers, a_pChunk->entityData[_i], _resolve.joltCorrection, 4);
				}
			}

			// ---- 比較 ----
			// 旧は動く敵のメッシュからも押し出すが、どの相手から押されたかは取れないので、
			// 敵に触れているときのずれも「ずれ」として数えている(Phase 4 までは出ることがある)
			if (_migration.compareQueries)
			{
				static App::Systems::PhysicsCompare::Stats s_stats{ "ResolveSphere" };
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const SphereResolve& _resolve = s_resolves[_i];
					++s_stats.queries;

					const bool _isSame = (_resolve.oldHit == _resolve.joltHit) &&
						App::Systems::PhysicsCompare::IsClose(_resolve.oldCorrection, _resolve.joltCorrection, _migration.compareTolerance);
					if (_isSame) continue;

					++s_stats.mismatches;
					if (App::Systems::PhysicsCompare::ShouldLogDetail(s_stats))
					{
						ENGINE_LOG("[PhysicsCompare] ResolveSphere mismatch entity=%llu center=(%.3f,%.3f,%.3f) old=%d(%.4f,%.4f,%.4f) jolt=%d(%.4f,%.4f,%.4f)",
							a_pChunk->entityData[_i],
							_resolve.center.x, _resolve.center.y, _resolve.center.z,
							_resolve.oldHit ? 1 : 0, _resolve.oldCorrection.x, _resolve.oldCorrection.y, _resolve.oldCorrection.z,
							_resolve.joltHit ? 1 : 0, _resolve.joltCorrection.x, _resolve.joltCorrection.y, _resolve.joltCorrection.z);
					}
				}
				App::Systems::PhysicsCompare::Report(s_stats);
			}

			// ---- 結果を反映 ----
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const SphereColliderComponent& _sphere = a_sphereArray[_i];
				LocalTransformComponent& _trans = a_transArray[_i];
				const SphereResolve& _resolve = s_resolves[_i];

				const bool _isHit = _migration.useJoltStaticQueries ? _resolve.joltHit : _resolve.oldHit;
				const Math::Vector3 _correction = _isHit
					? (_migration.useJoltStaticQueries ? _resolve.joltCorrection : _resolve.oldCorrection)
					: Math::Vector3{};

				// 補正をトランスフォームへ反映
				if (_isHit)
				{
					_trans.pos.x += _correction.x;
					_trans.pos.y += _correction.y;
					_trans.pos.z += _correction.z;
					_trans.isDirty = true;
				}

				// デバッグ描画（押し出しが起きたら赤、なければ緑）。押し出し後の中心で描画。
				DirectX::BoundingSphere _drawSphere;
				_drawSphere.Center = _resolve.center + _correction;
				_drawSphere.Radius = _sphere.radius;
				a_ctx.pServices->pDebugDraw->DrawSphere(
					_drawSphere, _isHit ? Engine::Color::RED : Engine::Color::GREEN);
			}
		}
	);
}
