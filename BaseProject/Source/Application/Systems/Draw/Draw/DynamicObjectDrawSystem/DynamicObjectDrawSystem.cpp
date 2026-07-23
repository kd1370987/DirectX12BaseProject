#include "DynamicObjectDrawSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/MainEngine.h"
#include "../../../../../Engine/Graphics/GraphicEngine.h"

#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Transform/PreviousWorldMatrixComponent.h"
#include "Application/Components/Resource/ModelComponent.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Application/Components/Resource/AnimatorComponent.h"

void DynamicObjectDrawSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const WorldMatrixComponent,const PreviousWorldMatrixComponent, const ModelComponent>(
		Engine::ECS::ESystemType::Draw,
			"DynamicObjectDrawSystem",
			[]
			(
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				ActiveTag* a_tags,
				const WorldMatrixComponent* a_worldMatArray,
				const PreviousWorldMatrixComponent* a_prevWorldMatArray,
				const ModelComponent* a_modelArray
				)
			{
				// グラフィックエンジン取得
				auto* _pGE = Engine::MainEngine::Instance().RefGraphicsEngine();
				if (!_pGE) return;

				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const WorldMatrixComponent& _worldMatComp = a_worldMatArray[_i];
					const PreviousWorldMatrixComponent& _prevWorldMatComp = a_prevWorldMatArray[_i];
					const ModelComponent& _modelComp = a_modelArray[_i];

					// モデル取得
					auto* _model = Engine::Resource::ResourceManager::Instance().Get(_modelComp.handle);
					if (!_model) continue;

					// 描画
					_pGE->SubmitModel(
						*a_ctx.pWorld,
						_model,
						_worldMatComp.worldMat,
						_modelComp.colorScale,
						_modelComp.emissiveScale
					);
				}
			},
			Engine::ECS::Exclude<AnimatorComponent>()
		);
}
