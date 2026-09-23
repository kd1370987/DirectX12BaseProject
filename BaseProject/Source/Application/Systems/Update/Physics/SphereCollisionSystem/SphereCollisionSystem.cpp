#include "SphereCollisionSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Collision/SphereCollider.h"
#include "Application/Components/Collision/Collider.h"
#include "Application/Components/Transform/LocalTransformComponent.h"

#include "Engine/MainEngine.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Graphics/DebugDraw/DebugDraw.h"
#include "Engine/Common/Color.h"

void SphereCollisionSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<const SphereColliderComponent, LocalTransformComponent>(
		Engine::ECS::ESystemType::Physics,
		"SphereCollisionSystem",
		[](
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_activeTag,
			const SphereColliderComponent* a_sphereArray,
			LocalTransformComponent* a_transArray
			)
		{
			ENGINE_PROFILE_SCOPE("Physics_ResolveSphere");
			const auto& _physicsWorld = a_ctx.pWorld->GetResource<Engine::Physics::PhysicsWorld>();

			// コライダーを持つかはチャンク(同じ組み合わせ)で揃っているので、先頭で1回だけ見る
			const bool _hasCollider = a_count > 0 &&
				a_ctx.pWorld->HasComponent<ColliderComponent>(a_pChunk->entityData[0]);

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const Engine::ECS::Entity _self = a_pChunk->entityData[_i];

				// 物理解決しないもの(isPhysical = 0)は押し出さない。
				// 群れのボスのボイド(Layer::Enemy)は当たりを受けるためだけに球を持っていて、
				// ここで押し出すと地形をすり抜けられず、地面に潜れなくなる
				if (_hasCollider)
				{
					const auto* _pColl = a_ctx.pWorld->RefData<ColliderComponent>(_self);
					if (_pColl && !_pColl->isPhysical) continue;
				}

				const SphereColliderComponent& _sphere = a_sphereArray[_i];
				LocalTransformComponent& _trans = a_transArray[_i];

				// 中心
				Math::Vector3 _center = Math::Vector3(_trans.pos) + Math::Vector3(_sphere.offset);

				// 地形と動く敵の判定メッシュから押し出す（_center は押し出し後に更新される）。
				// 弾・ボイドの箱からは押し出さない(ボイド同士の離れは Boid の separation の仕事)
				Math::Vector3 _correction = {};
				bool _isHit = _physicsWorld.ResolveSphere(
					_center, _sphere.radius,
					Engine::Physics::kQueryAllLayers, _self, _correction, 4);

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
				_drawSphere.Center = _center;
				_drawSphere.Radius = _sphere.radius;
				a_ctx.pServices->pDebugDraw->DrawSphere(
					_drawSphere, _isHit ? Engine::Color::RED : Engine::Color::GREEN);
			}
		}
	);
}
