#include "ModelFixupSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Core/PhaseTag/PostDeserializeTag.h"
#include "Application/Components/Render/ModelComponent.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace App::System
{
	void ModelFixupSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.PostDeserializeTask<Component::ModelComponent>(
			Engine::ECS::ESystemType::PostDeserialize,
			"ModelFixupSystem",
			[](
				Engine::ECS::Chunk* a_pChunk,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::PostDeserializeTag* a_tag,
				Component::ModelComponent* a_modelArray
			)
			{
				for(size_t _i = 0; _i < a_count; ++_i)
				{
					Component::ModelComponent& _modelComp = a_modelArray[_i];

					// モデルの読み込みを要求する。
					// 実体の到着は待たない : ここで待つとシーン読み込みでメインスレッドが止まる。
					// 届くまでは ModelReadyGateSystem がこのエンティティを
					// Start フェーズへ進めないので、Start 系は揃ってから1回だけ走る
					if(_modelComp.modelGUID != Core::DEFAULT_GUID)
					{
						a_ctx.pServices->pResourceManager->AcquireRequest(_modelComp.handle, _modelComp.modelGUID);
					}
				}
			}
		);
	}
}

