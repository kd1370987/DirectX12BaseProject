#include "BoxCollisionSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Collision/BoxCollider.h"
#include "Application/Components/Transform/LocalTransformComponent.h"

#include "Engine/MainEngine.h"
#include "Engine/Collision/CollisionWorld.h"
#include "Engine/Editor/Editor.h"
#include "Engine/Common/Color.h"

void BoxCollisionSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const BoxColliderComponent, const LocalTransformComponent>(
		Engine::ECS::ESystemType::Physics,
		"BoxCollisionSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_activeTag,
			const BoxColliderComponent* a_boxArray,
			const LocalTransformComponent* a_transArray
			)
		{
			auto* _pCollWorld = Engine::MainEngine::Instance().RefCollisionWorld();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const BoxColliderComponent& _box = a_boxArray[_i];
				const LocalTransformComponent& _trans = a_transArray[_i];

				// 中心（ワールド軸に平行なAABB）
				DXSM::Vector3 _center = DXSM::Vector3(_trans.pos) + DXSM::Vector3(_box.offset);

				// 重なり判定
				Engine::Collision::BoxInfo _info;
				_info.center = _center;
				_info.extents = _box.extents;

				Engine::Collision::Result _res = {};
				bool _isHit = _pCollWorld->VsBox(_info, _res, a_pChunk->entityData[_i]);

				// デバッグ描画（重なっていれば赤、なければ緑）
				DirectX::BoundingBox _drawBox;
				_drawBox.Center = _center;
				_drawBox.Extents = _box.extents;
				Engine::Editor::MainEditor::Instance().DrawBox(
					_drawBox, _isHit ? Engine::Color::RED : Engine::Color::GREEN);
			}
		}
	);
}
