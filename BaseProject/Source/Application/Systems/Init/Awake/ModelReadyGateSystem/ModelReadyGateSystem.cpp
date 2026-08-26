#include "ModelReadyGateSystem.h"

#include "Application/ECS/World/World.h"

#include "../../../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../../../Components/Resource/ModelComponent.h"
#include "../../../../InstanceResource/ResourceWaitResource.h"

void ModelReadyGateSystem::Init(App::ECS::World& a_world)
{
	a_world.AwakeTask<const ModelComponent>(
		Engine::ECS::ESystemType::Awake,
		"ModelReadyGateSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			AwakeTag* a_awakeTag,
			const ModelComponent* a_pModelArray
		)
		{
			auto& _wait = a_ctx.pWorld->GetResource<ResourceWaitResource>();
			auto& _resMgr = *a_ctx.pServices->pResourceManager;

			for (uint32_t _i = 0; _i < a_count; ++_i)
			{
				const ModelComponent& _modelComp = a_pModelArray[_i];

				// そもそもモデルを使わないエンティティは待たせない
				if (_modelComp.modelGUID == Engine::DefaultGUID) continue;

				// 待つのは読込中のときだけ。
				// Failed をここで待たせると、もう届かないものを永久に待って
				// Start が一生走らなくなる
				const auto _state = _resMgr.GetState(_modelComp.handle);
				if (_state != Engine::Resource::EResourceState::Loading) continue;

				_wait.AddWait(a_pChunk->entityData[_i]);
			}
		}
	);
}
