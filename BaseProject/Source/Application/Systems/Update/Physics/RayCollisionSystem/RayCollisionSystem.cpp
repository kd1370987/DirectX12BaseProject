#include "RayCollisionSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Collision/Collider.h"
#include "Application/Components/Collision/RayCollider.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"
#include "Application/Components/Collision/GroundStateComponent.h"

#include "Engine/MainEngine.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Graphics/DebugDraw/DebugDraw.h"
#include "Engine/Common/Color.h"


//==========================================================================================
// RayCollisionSystem
//
// 足元へレイを1本撃って地面へスナップさせ、接地判定を GroundStateComponent へ書く。
//
// ・GroundStateComponent はプレハブに入れなくてよい。RayColliderComponent を持つものへ
//   Start フェーズで自動で足す(下の GroundStateAttachSystem)。
//   Start の時点ではまだ ActiveTag が付いていないので、足しても初期化のやり直しにはならない。
// ・以前は接地判定を StateMachineComponent::isGround に書いていた。
//   ステートマシンの書き手が増えると読みたいだけの側の宣言がぶつかるので、分けてある。
//==========================================================================================
void RayCollisionSystem::Init(App::ECS::APPWorld& a_world)
{
	//--------------------------------------------------------------------------
	// 接地判定の置き場を付ける(持っていないものだけ)
	//--------------------------------------------------------------------------
	a_world.StartTask<const RayColliderComponent>(
		Engine::ECS::ESystemType::Start,
		"GroundStateAttachSystem",
		[](
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			StartTag*,
			const RayColliderComponent*
			)
		{
			const auto _typeID = a_ctx.pWorld->GetCompTypeID<GroundStateComponent>();
			if (!Engine::ECS::IsValidTypeID(_typeID)) return;

			// 反復中なので予約する。反映は Start フェーズの直後(ActiveTag への遷移の前)
			for (uint32_t _i = 0; _i < a_count; ++_i)
			{
				a_ctx.pWorld->ReserveAddComponent(_typeID, a_pChunk->entityData[_i]);
			}
		},
		Engine::ECS::Exclude<GroundStateComponent>{}
	);

	//--------------------------------------------------------------------------
	// 足元のレイ
	//--------------------------------------------------------------------------
	a_world.ActiveTask<const ColliderComponent, const RayColliderComponent, LocalTransformComponent, VelocityComponent, GroundStateComponent>(
		Engine::ECS::ESystemType::Physics,
		"RayCollisionSystem",
		[](
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_activeTag,
			const ColliderComponent* a_collArray,
			const RayColliderComponent* a_rayArray,
			LocalTransformComponent* a_transArray,
			VelocityComponent* a_velArray,
			GroundStateComponent* a_groundArray
			)
		{
			ENGINE_PROFILE_SCOPE("Physics_GroundRay");
			const auto& _physicsWorld = a_ctx.pWorld->GetResource<Engine::Physics::PhysicsWorld>();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				LocalTransformComponent& _trans = a_transArray[_i];
				VelocityComponent& _vel = a_velArray[_i];
				GroundStateComponent& _ground = a_groundArray[_i];
				const RayColliderComponent& _ray = a_rayArray[_i];

				// 足元原点。「段差許容分だけ上」を発射点にして真下へ1本撃つ。
				// カバー範囲 : [足元 - snapDown, 足元 + stepUp]
				Math::Ray _info;
				_info.origin = _trans.pos;
				_info.origin.y += _ray.stepUp;					// 段差分だけ上げる（浮かしも兼ねる）
				_info.direction = { 0.0f, -1.0f, 0.0f };
				_info.maxDistance = _ray.stepUp + _ray.snapDown;	// 上下をまとめて1本

				// 静的・動くもの(敵・弾・ボイド)のどれにも当たる。自分自身は除く
				Engine::Physics::RayHit _hit = {};
				const bool _isHit = _physicsWorld.CastRay(
					_info, Engine::Physics::kQueryAllLayers, a_pChunk->entityData[_i], _hit);

				// プローブのデバッグ表示（緑=接地, 赤=空中。終点に球）
				a_ctx.pServices->pDebugDraw->DrawRay(
					_info.origin, _info.direction, _info.maxDistance, _isHit,
					_isHit ? Engine::Color::GREEN : Engine::Color::RED);

				// 範囲内に地面が無い → 空中
				if (!_isHit)
				{
					_ground.isGround = false;
					continue;
				}

				// ジャンプ上昇中はスナップしない（頭上の段差に吸い付かないように）
				if (_vel.value.y > 0.0f)
				{
					_ground.isGround = false;
					continue;
				}

				// 足元を地面へスナップ（段差登り／下り吸着 の両方をこれ1つで表現）
				_trans.pos.y = _hit.position.y;
				_trans.isDirty = true;
				_vel.value.y = 0.0f;
				_ground.isGround = true;
			}
		}
	);
}
