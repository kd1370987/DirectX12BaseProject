#include "RayCollisionSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Collision/Collider.h"
#include "Application/Components/Collision/RayCollider.h"
#include "Application/Components/Transform/TransformComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"
#include "../../../../Components/Resource/StateMachineComponent.h"
#include "Application/Components/Resource/ModelComponent.h"


#include "Engine/MainEngine.h"
#include "Engine/Collision/CollisionWorld.h"

#include "Engine/Collision/Collision.h"


void RayCollisionSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ColliderComponent, const RayColliderComponent, TransformComponent,VelocityComponent, StateMachineComponent>(
		Engine::ECS::ESystemType::Physics,
		"RayCollisionSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_startTag,
			const ColliderComponent* a_collArray,
			const RayColliderComponent* a_rayArray,
			TransformComponent* a_transArray,
			VelocityComponent* a_velArray,
			StateMachineComponent* a_stateArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				TransformComponent& _transComp = a_transArray[_i];
				VelocityComponent& _velComp = a_velArray[_i];
				StateMachineComponent& _stateComp = a_stateArray[_i];
				const RayColliderComponent& _rayComp = a_rayArray[_i];

				float _test = 1.0f;

				// レイ情報作成
				Engine::Collision::RayInfo _info = {};
				_info.origin = _transComp.pos;
				_info.origin.y += _rayComp.pos.y;
				_info.maxDistance = _rayComp.length;
				_info.direction = _rayComp.dir;

				// ヒットリザルト
				Engine::Collision::Result _res = {};

				// コリジョンワールドの取得
				auto* _pCollWorld = Engine::MainEngine::Instance().RefCollisionWorld();

				// レイ判定
				if (!_pCollWorld->Raycast(_info, _res, a_pChunk->entityData[_i]))
				{
					_stateComp.isGround = false;
					continue;
				}
				
				if (_velComp.value.y > 0) continue;

				float _groundDistance = _transComp.pos.y - _res.hitPos.y;
				if (_groundDistance > 0.05f) continue;
				_transComp.pos.y -= _groundDistance;
				_velComp.value.y = 0;
				_stateComp.isGround = true;

				Engine::Editor::MainEditor::Instance().AddLog("_res.hitPos\n");
				Engine::Editor::MainEditor::Instance().AddLogVector(&_res.hitPos.x,3);

			}
		}
	);
}
