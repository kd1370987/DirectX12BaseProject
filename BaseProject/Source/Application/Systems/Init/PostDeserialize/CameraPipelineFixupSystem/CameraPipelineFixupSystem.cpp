#include "CameraPipelineFixupSystem.h"

#include "Application/ECS/World/World.h"

#include "Application/Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "Application/Components/Camera/CameraParamComponent.h"

void CameraPipelineFixupSystem::Init(App::ECS::World& a_world)
{
	a_world.PostDeserializeTask<CameraParamComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"CameraPipelineFixupSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			PostDeserializeTag* a_tag,
			CameraParamComponent* a_array
			)
		{
			for (uint32_t _i = 0; _i < a_count; ++_i)
			{
				CameraParamComponent& _comp = a_array[_i];

				// 描画構成を指していないカメラは従来経路のまま
				if (_comp.pipelineGUID == Engine::DefaultGUID) continue;

				a_ctx.pServices->pResourceManager->AcquireImmediate(_comp.pipelineHandle, _comp.pipelineGUID);
			}
		}
	);
}
