#include "RayCollisionSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Collision/Collider.h"
#include "Application/Components/Collision/RayCollider.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"
#include "../../../../Components/Resource/StateMachineComponent.h"

#include "Engine/MainEngine.h"
#include "Engine/Collision/CollisionWorld.h"
#include "Engine/Editor/Editor.h"
#include "Engine/Common/Color.h"


void RayCollisionSystem::Init(Engine::ECS::World& a_world)
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
			auto* _pCollWorld = Engine::MainEngine::Instance().RefCollisionWorld();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				LocalTransformComponent& _trans = a_transArray[_i];
				VelocityComponent& _vel = a_velArray[_i];
				StateMachineComponent& _state = a_stateArray[_i];
				const RayColliderComponent& _ray = a_rayArray[_i];

				// 足元原点。「段差許容分だけ上」を発射点にして真下へ1本撃つ。
				// カバー範囲 : [足元 - snapDown, 足元 + stepUp]
				Engine::Collision::RayInfo _info;
				_info.origin = _trans.pos;
				_info.origin.y += _ray.stepUp;					// 段差分だけ上げる（浮かしも兼ねる）
				_info.direction = { 0.0f, -1.0f, 0.0f };
				_info.maxDistance = _ray.stepUp + _ray.snapDown;	// 上下をまとめて1本

				Engine::Collision::Result _res = {};
				bool _isHit = _pCollWorld->Raycast(_info, _res, a_pChunk->entityData[_i]);

				// プローブのデバッグ表示（緑=接地, 赤=空中。終点に球）
				Engine::Editor::MainEditor::Instance().DrawRay(
					_info.origin, _info.direction, _info.maxDistance, _isHit,
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
				_trans.pos.y = _res.hitPos.y;
				_trans.isDirty = true;
				_vel.value.y = 0.0f;
				_state.isGround = true;
			}
		}
	);
}
