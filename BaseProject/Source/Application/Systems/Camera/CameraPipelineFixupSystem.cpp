#include "CameraPipelineFixupSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Core/PhaseTag/PostDeserializeTag.h"
#include "Application/Components/Camera/CameraParamComponent.h"

namespace App::System
{
	void CameraPipelineFixupSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.PostDeserializeTask<Component::CameraParamComponent>(
			Engine::ECS::ESystemType::PostDeserialize,
			"CameraPipelineFixupSystem",
			[]
			(
				Engine::ECS::Chunk* /*a_pChunk*/,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::PostDeserializeTag* /*a_tag*/,
				Component::CameraParamComponent* a_array
				)
			{
				for (uint32_t _i = 0; _i < a_count; ++_i)
				{
					Component::CameraParamComponent& _comp = a_array[_i];

					// 描画構成を指していないカメラは従来経路のまま
					if (_comp.pipelineGUID == Core::DEFAULT_GUID) continue;

					a_ctx.pServices->pResourceManager->AcquireImmediate(_comp.pipelineHandle, _comp.pipelineGUID);
				}
			}
		);
	}
}
