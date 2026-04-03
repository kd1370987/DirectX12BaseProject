#include "CamSetShaderSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Tag/CameraTag.h"
#include "Application/Components/Tag/ActiveCameraTag.h"

#include "Application/Components/Transform/WorldMatrixComponent.h"

#include "Application/Components/Camera/CameraParamComponent.h"
#include "Application/Components/Camera/FocusParamComponent.h"
#include "Application/Components/Camera/ProjMatComponent.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

void CamSetShaderSystem::Run(Engine::ECS::World& a_world, float a_dt)
{
	a_world.ForEach<ActiveCameraTag,CameraTag, ProjMatComponent, WorldMatrixComponent>(
		[&a_world, a_dt]
		(
			Engine::ECS::ArchetypeChunk* a_archeChunk,
			size_t a_count,
			ActiveCameraTag* a_aTag,
			CameraTag* a_cTag,
			ProjMatComponent* a_projMatArray,
			WorldMatrixComponent* a_wMatArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				RenderContext::Instance().SetProjectionMatrix(a_projMatArray[_i].projMat);

				RenderContext::Instance().SetToShader(a_wMatArray[_i].worldMat);
			}
		}
	);
}
