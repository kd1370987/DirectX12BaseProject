#include "CamSetShaderSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Tag/CameraTag.h"
#include "Application/Components/Tag/ActiveCameraTag.h"

#include "Application/Components/Transform/WorldMatrixComponent.h"

#include "Application/Components/Camera/CameraParamComponent.h"
#include "Application/Components/Camera/FocusParamComponent.h"
#include "Application/Components/Camera/ProjMatComponent.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

void CamSetShaderSystem::Init(Engine::ECS::World& a_world)
{
	a_world.RegisterTask<const ActiveCameraTag, const CameraTag, const ProjMatComponent, const WorldMatrixComponent>(
		Engine::ECS::ESystemType::PreDraw,
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			const ActiveCameraTag* a_activeCamTagArray,
			const CameraTag* a_camTagArray,
			const ProjMatComponent* a_projMatArray,
			const WorldMatrixComponent* a_worldMatArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ProjMatComponent& _projMatComp = a_projMatArray[_i];
				const WorldMatrixComponent& _worldMatComp = a_worldMatArray[_i];

				Engine::Graphics::RenderContext::Instance().SetProjectionMatrix(_projMatComp.projMat);
				Engine::Graphics::RenderContext::Instance().SetToShader(_worldMatComp.worldMat);
			}
		}
	);
}