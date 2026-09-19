#include "CapsuleCollisionSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Collision/CapsuleCollider.h"
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
	// カプセルをHLSLのカプセル形状（DrawCapsule）で描画する。
	//
	// ベースメッシュ（GetCapsulePoint）は次の単位カプセル :
	//   ・半径          r = 0.5
	//   ・円柱の半長     h = 0.5 （＝上下の球中心が y = ±0.5）
	// これを 半径 a_radius・球中心間の距離 a_height に合わせてスケールする。
	//   ・XZ … 半径を合わせる      : 0.5 -> a_radius        => scale = a_radius * 2
	//   ・Y  … 球中心間を合わせる  : 1.0(=±0.5) -> a_height => scale = a_height
	// ※ ベースが「半球半径 == 円柱半長」固定比のため、a_height != a_radius*2 のときは
	//    上下のキャップが楕円に伸びる（当たり判定の線分自体は常に一致）。
	void DrawCapsuleUpright(
		Engine::Graphics::DebugDraw* a_pDebugDraw,
		const Math::Vector3& a_center,
		float a_radius,
		float a_height,
		const Math::Color& a_color)
	{
		if (!a_pDebugDraw) return;
		Math::Matrix _mat =
			Math::Matrix::CreateScale(a_radius * 2.0f, a_height, a_radius * 2.0f) *
			Math::Matrix::CreateTranslation(a_center);
		a_pDebugDraw->DrawCapsule(_mat, a_color);
	}

	// 1体ぶんの押し出し。旧と Jolt の結果を並べて持つ
	struct CapsuleResolve
	{
		Math::Vector3 center = {};

		bool oldHit = false;
		Math::Vector3 oldCorrection = {};

		bool joltHit = false;
		Math::Vector3 joltCorrection = {};
	};
}

void CapsuleCollisionSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<const CapsuleColliderComponent, LocalTransformComponent>(
		Engine::ECS::ESystemType::Physics,
		"CapsuleCollisionSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_activeTag,
			const CapsuleColliderComponent* a_capArray,
			LocalTransformComponent* a_transArray
			)
		{
			// 旧(CollisionWorld)/ Jolt(PhysicsWorld)のどちらで判定するか。移行中だけの切り替え
			const auto& _migration = a_ctx.pServices->pOptionManager->GetPhysicsMigrationOption();

			// ECS はシングルスレッドなので置き場は使い回す
			static std::vector<CapsuleResolve> s_resolves;
			s_resolves.assign(a_count, {});

			// 中心（ワールドY軸方向の直立カプセル）
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				s_resolves[_i].center = Math::Vector3(a_transArray[_i].pos) + Math::Vector3(a_capArray[_i].offset);
			}

			// 線分の端点(下端・上端の球中心)
			auto _makeSegment = [&](size_t a_i, Math::Vector3& a_outA, Math::Vector3& a_outB)
			{
				const Math::Vector3 _half = { 0.0f, a_capArray[a_i].height * 0.5f, 0.0f };
				a_outA = s_resolves[a_i].center - _half;	// 下端の球中心
				a_outB = s_resolves[a_i].center + _half;	// 上端の球中心
			};

			// ---- 旧 : 静的・動的の両方のツリーのメッシュから押し出す ----
			if (_migration.RunsOld())
			{
				ENGINE_PROFILE_SCOPE("Collision_ResolveCapsule");
				auto& _collWorld = a_ctx.pWorld->GetResource<Engine::Collision::CollisionWorld>();
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					Math::Vector3 _pointA, _pointB;
					_makeSegment(_i, _pointA, _pointB);
					CapsuleResolve& _resolve = s_resolves[_i];
					_resolve.oldHit = _collWorld.ResolveCapsule(
						_pointA, _pointB, a_capArray[_i].radius, a_pChunk->entityData[_i], _resolve.oldCorrection, 4);
				}
			}

			// ---- Jolt : 今は静的なボディだけ(動くものは Phase 4 で入る)。レイヤーは旧と同じく全部 ----
			if (_migration.RunsJolt())
			{
				ENGINE_PROFILE_SCOPE("Physics_ResolveCapsule");
				const auto& _physicsWorld = a_ctx.pWorld->GetResource<Engine::Physics::PhysicsWorld>();
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					Math::Vector3 _pointA, _pointB;
					_makeSegment(_i, _pointA, _pointB);
					CapsuleResolve& _resolve = s_resolves[_i];
					_resolve.joltHit = _physicsWorld.ResolveCapsule(
						_pointA, _pointB, a_capArray[_i].radius,
						Engine::Physics::kQueryAllLayers, a_pChunk->entityData[_i], _resolve.joltCorrection, 4);
				}
			}

			// ---- 比較 ----
			// 旧は動く敵のメッシュからも押し出すが、どの相手から押されたかは取れないので、
			// 敵に触れているときのずれも「ずれ」として数えている(Phase 4 までは出ることがある)
			if (_migration.compareQueries)
			{
				static App::Systems::PhysicsCompare::Stats s_stats{ "ResolveCapsule" };
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const CapsuleResolve& _resolve = s_resolves[_i];
					++s_stats.queries;

					const bool _isSame = (_resolve.oldHit == _resolve.joltHit) &&
						App::Systems::PhysicsCompare::IsClose(_resolve.oldCorrection, _resolve.joltCorrection, _migration.compareTolerance);
					if (_isSame) continue;

					++s_stats.mismatches;
					if (App::Systems::PhysicsCompare::ShouldLogDetail(s_stats))
					{
						ENGINE_LOG("[PhysicsCompare] ResolveCapsule mismatch entity=%llu center=(%.3f,%.3f,%.3f) old=%d(%.4f,%.4f,%.4f) jolt=%d(%.4f,%.4f,%.4f)",
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
				const CapsuleColliderComponent& _cap = a_capArray[_i];
				LocalTransformComponent& _trans = a_transArray[_i];
				const CapsuleResolve& _resolve = s_resolves[_i];

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
				DrawCapsuleUpright(
					a_ctx.pServices->pDebugDraw,
					_resolve.center + _correction, _cap.radius, _cap.height,
					_isHit ? Engine::Color::RED : Engine::Color::GREEN);
			}
		}
	);
}
