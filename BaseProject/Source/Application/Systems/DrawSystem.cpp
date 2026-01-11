#include "DrawSystem.h"

#include "Engine/ECS/World/World.h"

#include "../Components/Transform/WorldMatrixComponent.h"
#include "../Components/Resource/ModelComponent.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/ResourceManager/ResourceManager.h"

void DrawSystem::Run(World& a_world, float a_dt)
{
	RenderContext::Instance().BeginSimpleRender();

	a_world.ForEach<WorldMatrixComponent,ModelComponent>(
		[&a_world, a_dt]
		(
			ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			WorldMatrixComponent* a_matArray,
			ModelComponent* a_modelArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				WorldMatrixComponent& _worldMatComp = a_matArray[_i];
				ModelComponent& _modelComp = a_modelArray[_i];

				RenderContext::Instance().DrawModel(
					_modelComp.modelID,
					_worldMatComp.worldMat,
					_modelComp.colorScale,
					_modelComp.emissiveScale
				);
			}
		}
	);
}
