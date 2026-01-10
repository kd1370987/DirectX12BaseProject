#include "DrawSystem.h"

#include "Engine/ECS/World/World.h"

#include "../Components/TransformComponent.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/ResourceManager/ResourceManager.h"

void DrawSystem::Run(World& a_world, float a_dt)
{
	RenderContext::Instance().BeginSimpleRender();

	a_world.ForEach<TransformComponent>(
		[&a_world, a_dt]
		(
			ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			TransformComponent* a_array
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				TransformComponent& _comp = a_array[_i];

				auto _wpModel2 = ResourceManager::Instance().GetModel("Asset/Model/Alicia/FBX/Alicia_solid_Unity.FBX");

				RenderContext::Instance().DrawModel(
					_wpModel2.lock(),
					_comp.worldMat,
					DirectX::XMFLOAT4{ 0.0f,1.0f,1.0f,1.0f },
					DirectX::XMFLOAT3{ 1.0f,1.0f,1.0f }
				);
			}
		}
	);
}
