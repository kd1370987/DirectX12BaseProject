#include "SerchGroundSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "../../../../Components/Character/SerchGroundComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Collision/Collider.h"

#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Graphics/DebugDraw/DebugDraw.h"
#include "Engine/Common/Color.h"

//==============================================================================
// SerchGroundSystem
//
// SerchGroundComponent を持つものの真上と真下へレイを打ち、地表の高さと
// 地面の中に居るかを書く。使う側(ワームボスのアッパー攻撃など)は結果を読むだけ。
//
// ・地中の判定は真上のレイを優先する。上向きの面の裏に当たった = 地表の下に居る。
// ・そうでなければ真下のレイで地表を取る(地上に居る)。
// ・どちらにも当たらなければ見つからなかった扱い(高さは前の値のまま)。
// ・位置は LocalTransform をそのまま使う。親を持たないもの(リーダー)が前提。
//==============================================================================
void SerchGroundSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<SerchGroundComponent, const LocalTransformComponent>(
		Engine::ECS::ESystemType::Update,
		"SerchGroundSystem",
		[](
			Engine::ECS::Chunk*               a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			SerchGroundComponent*             a_serchArray,
			const LocalTransformComponent*    a_transArray
		)
		{
			ENGINE_PROFILE_SCOPE("Physics_SerchGroundRay");
			const auto& _physicsWorld = a_ctx.pWorld->GetResource<Engine::Physics::PhysicsWorld>();

			// 地面として見るのは静的なものだけ
			const uint32_t _queryMask = static_cast<uint32_t>(Layer::StaticObject);

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				SerchGroundComponent& _serch = a_serchArray[_i];
				const Engine::ECS::Entity _self = a_pChunk->entityData[_i];

				Math::Ray _upRay;
				_upRay.origin      = a_transArray[_i].pos;
				_upRay.direction   = { 0.0f, 1.0f, 0.0f };
				_upRay.maxDistance = _serch.maxDistance;

				Math::Ray _downRay = _upRay;
				_downRay.direction = { 0.0f, -1.0f, 0.0f };

				Engine::Physics::RayHit _upHit   = {};
				Engine::Physics::RayHit _downHit = {};
				const bool _isUpHit   = _physicsWorld.CastRay(_upRay, _queryMask, _self, _upHit);
				const bool _isDownHit = _physicsWorld.CastRay(_downRay, _queryMask, _self, _downHit);

				// 真上で上向きの面(の裏)に当たった = 地表の下
				const bool _isUnderGround = _isUpHit && _upHit.normal.y > 0.0f;

				if (_isUnderGround)
				{
					_serch.isFoundGround = 1;
					_serch.isUnderGround = 1;
					_serch.groundHeight  = _upHit.position.y;
				}
				else if (_isDownHit)
				{
					_serch.isFoundGround = 1;
					_serch.isUnderGround = 0;
					_serch.groundHeight  = _downHit.position.y;
				}
				else
				{
					_serch.isFoundGround = 0;
					_serch.isUnderGround = 0;
				}

				// デバッグ表示(青=地中で使った上のレイ, 緑=地上で使った下のレイ, 赤=外れ)
				if (a_ctx.pServices && a_ctx.pServices->pDebugDraw)
				{
					auto& _debugDraw = *a_ctx.pServices->pDebugDraw;
					_debugDraw.DrawRay(_upRay.origin, _upRay.direction, _upRay.maxDistance, _isUpHit,
						_isUnderGround ? Engine::Color::BLUE : Engine::Color::RED);
					_debugDraw.DrawRay(_downRay.origin, _downRay.direction, _downRay.maxDistance, _isDownHit,
						(!_isUnderGround && _isDownHit) ? Engine::Color::GREEN : Engine::Color::RED);
				}
			}
		}
	);
}
