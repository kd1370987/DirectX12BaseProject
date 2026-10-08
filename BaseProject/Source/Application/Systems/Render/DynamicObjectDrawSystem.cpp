#include "DynamicObjectDrawSystem.h"

#include "Application/ECS/World/APPWorld.h"
#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicsEngine.h"

#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Transform/PreviousWorldMatrixComponent.h"
#include "Application/Components/Render/ModelComponent.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Application/Components/Animation/AnimatorComponent.h"

namespace App::System
{
	void DynamicObjectDrawSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.ActiveTask<const Component::WorldMatrixComponent,const Component::PreviousWorldMatrixComponent, const Component::ModelComponent>(
			Engine::ECS::ESystemType::Draw,
				"DynamicObjectDrawSystem",
				[]
				(
					Engine::ECS::Chunk* /*a_pChunk*/,
					uint32_t a_count,
					const Engine::ECS::SystemContext& a_ctx,
					Component::ActiveTag* /*a_tags*/,
					const Component::WorldMatrixComponent* a_worldMatArray,
					const Component::PreviousWorldMatrixComponent* /*a_prevWorldMatArray*/,
					const Component::ModelComponent* a_modelArray
					)
				{
					// グラフィックエンジン取得
					auto* _pGE = a_ctx.pServices->pMainEngine->RefGraphicsEngine();
					if (!_pGE) return;

					for (size_t _i = 0; _i < a_count; ++_i)
					{
						const Component::WorldMatrixComponent& _worldMatComp = a_worldMatArray[_i];
							const Component::ModelComponent& _modelComp = a_modelArray[_i];

						// モデル取得
						auto* _model = a_ctx.pServices->pResourceManager->Get(_modelComp.handle);
						if (!_model) continue;

						// 描画
						_pGE->RefDrawSubmitter()->SubmitModel(
							*a_ctx.pWorld,
							_model,
							_worldMatComp.worldMat,
							_modelComp.colorScale,
							_modelComp.emissiveScale,
							_modelComp.GetEmissiveAdd()
						);
					}
				},
				Engine::ECS::Exclude<Component::AnimatorComponent>()
			);
	}
}
