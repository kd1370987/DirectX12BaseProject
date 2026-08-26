#include "ModelFixupSystem.h"

#include "Application/ECS/World/World.h"

#include "../../../../Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "../../../../Components/Resource/ModelComponent.h"
#include "../../../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"

void ModelFixupSystem::Init(App::ECS::World& a_world)
{
	a_world.PostDeserializeTask<ModelComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"ModelFixupSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			PostDeserializeTag* a_tag,
			ModelComponent* a_modelArray
		)
		{
			for(size_t _i = 0; _i < a_count; ++_i)
			{
				ModelComponent& _modelComp = a_modelArray[_i];

				// モデルの読み込みを要求する。
				// 実体の到着は待たない : ここで待つとシーン読み込みでメインスレッドが止まる。
				// 届くまでは ModelReadyGateSystem がこのエンティティを
				// Start フェーズへ進めないので、Start 系は揃ってから1回だけ走る
				if(_modelComp.modelGUID != Engine::DefaultGUID)
				{
					a_ctx.pServices->pResourceManager->AcquireRequest(_modelComp.handle, _modelComp.modelGUID);
				}
			}
		}
	);
}

