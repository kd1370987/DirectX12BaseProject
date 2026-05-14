#include "CommitWorldMatrixFromLocalSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Transform/TransformComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"

void CommitWorldMatrixFromLocalSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const LocalTransformComponent, WorldMatrixComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_activeTags,
			const LocalTransformComponent* a_transArray,
			WorldMatrixComponent* a_worldMatArray
		) 
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const LocalTransformComponent& _transComp = a_transArray[_i];
				WorldMatrixComponent& _wMatComp = a_worldMatArray[_i];

				DXSM::Matrix _tMat = DXSM::Matrix::CreateTranslation(_transComp.pos);
				DXSM::Matrix _rMat = DXSM::Matrix::CreateFromQuaternion(_transComp.quat);
				DXSM::Matrix _sMat = DXSM::Matrix::CreateScale(_transComp.scale);

				_wMatComp.worldMat = (_sMat * _rMat * _tMat);
			}
		}
	);
}
