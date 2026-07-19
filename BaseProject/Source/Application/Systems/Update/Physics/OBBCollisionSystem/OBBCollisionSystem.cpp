#include "OBBCollisionSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Collision/OBBCollider.h"
#include "Application/Components/Transform/LocalTransformComponent.h"

#include "Engine/MainEngine.h"
#include "Engine/Collision/CollisionWorld.h"
#include "Engine/Editor/Editor.h"
#include "Engine/Common/Color.h"

void OBBCollisionSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const OBBColliderComponent, const LocalTransformComponent>(
		Engine::ECS::ESystemType::Physics,
		"OBBCollisionSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_activeTag,
			const OBBColliderComponent* a_obbArray,
			const LocalTransformComponent* a_transArray
			)
		{
			auto* _pCollWorld = Engine::MainEngine::Instance().RefCollisionWorld();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const OBBColliderComponent& _obb = a_obbArray[_i];
				const LocalTransformComponent& _trans = a_transArray[_i];

				// 中心・向き（向きはエンティティの回転を使う）
				DXSM::Vector3 _center = DXSM::Vector3(_trans.pos) + DXSM::Vector3(_obb.offset);

				// 重なり判定
				Engine::Collision::OBBInfo _info;
				_info.center = _center;
				_info.extents = _obb.extents;
				_info.orientation = _trans.quat;

				Engine::Collision::Result _res = {};
				bool _isHit = _pCollWorld->VsOBB(_info, _res, a_pChunk->entityData[_i]);

				// デバッグ描画（重なっていれば赤、なければ緑）
				DirectX::BoundingOrientedBox _drawObb;
				_drawObb.Center = _center;
				_drawObb.Extents = _obb.extents;
				_drawObb.Orientation = _trans.quat;
				Engine::Editor::MainEditor::Instance().DrawBox(
					_drawObb, _isHit ? Engine::Color::RED : Engine::Color::GREEN);
			}
		}
	);
}
