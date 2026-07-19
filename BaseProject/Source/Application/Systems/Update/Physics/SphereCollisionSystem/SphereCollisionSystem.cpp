#include "SphereCollisionSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Collision/SphreCollider.h"
#include "Application/Components/Transform/LocalTransformComponent.h"

#include "Engine/MainEngine.h"
#include "Engine/Collision/CollisionWorld.h"
#include "Engine/Editor/Editor.h"
#include "Engine/Common/Color.h"

void SphereCollisionSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const SphereColliderComponent, LocalTransformComponent>(
		Engine::ECS::ESystemType::Physics,
		"SphereCollisionSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_activeTag,
			const SphereColliderComponent* a_sphereArray,
			LocalTransformComponent* a_transArray
			)
		{
			auto* _pCollWorld = Engine::MainEngine::Instance().RefCollisionWorld();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const SphereColliderComponent& _sphere = a_sphereArray[_i];
				LocalTransformComponent& _trans = a_transArray[_i];

				// 中心
				DXSM::Vector3 _center = DXSM::Vector3(_trans.pos) + DXSM::Vector3(_sphere.offset);

				// マップから押し出す（_center は押し出し後に更新される）
				DXSM::Vector3 _correction = {};
				bool _isHit = _pCollWorld->ResolveSphere(
					_center, _sphere.radius, a_pChunk->entityData[_i], _correction, 4);

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
				Engine::Editor::MainEditor::Instance().DrawSphere(
					_drawSphere, _isHit ? Engine::Color::RED : Engine::Color::GREEN);
			}
		}
	);
}
