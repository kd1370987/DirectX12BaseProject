#include "CamSetShaderSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/MainEngine.h"

#include "Application/Components/Tag/CameraTag.h"
#include "Application/Components/Tag/ActiveCameraTag.h"

#include "Application/Components/Transform/WorldMatrixComponent.h"

#include "Application/Components/Camera/CameraParamComponent.h"
#include "Application/Components/Camera/FocusParamComponent.h"
#include "Application/Components/Camera/ProjMatComponent.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "../../../../../Engine/Graphics/GraphicEngine.h"

void CamSetShaderSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ActiveCameraTag, const CameraTag, const ProjMatComponent, const WorldMatrixComponent>(
		Engine::ECS::ESystemType::PreDraw,
		"CamSetShaderSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const ActiveCameraTag* a_activeCamTagArray,
			const CameraTag* a_camTagArray,
			const ProjMatComponent* a_projMatArray,
			const WorldMatrixComponent* a_worldMatArray
		)
		{
			auto* _pRCT = Engine::MainEngine::Instance().RefRenderContext();
			auto* _pGE = Engine::MainEngine::Instance().RefGraphicsEngine();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ProjMatComponent& _projMatComp = a_projMatArray[_i];
				const WorldMatrixComponent& _worldMatComp = a_worldMatArray[_i];
				
				_pGE->SetCameraMat(_worldMatComp.worldMat);
				_pGE->SetProjMat(_projMatComp.projMat);
			}
		}
	);
}