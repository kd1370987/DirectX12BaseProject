#include "GroundEffectSetSystem.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicsEngine.h"

#include "Application/ECS/World/APPWorld.h"

#include "../../Components/Effect/GroundEffectTag.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Render/ModelComponent.h"

#include "Engine/Graphics/Frame/RenderContext/RenderContext.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Application/Components/Animation/AnimatorComponent.h"
namespace App::System
{
	void GroundEffectSetSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.ActiveTask<const Component::GroundEffectTag,const Component::WorldMatrixComponent, const Component::ModelComponent>(
			Engine::ECS::ESystemType::Draw,
			"GroundEffectSetSystem",
			[] 
			(
				Engine::ECS::Chunk* a_pChunk,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ActiveTag* a_tags,
				const Component::GroundEffectTag* a_groundEffectTags,
				const Component::WorldMatrixComponent* a_worldMatArray,
				const Component::ModelComponent* a_modelArray
			)
			{
				// グラフィックスエンジン取得
				auto* _pGE = a_ctx.pServices->pMainEngine->RefGraphicsEngine();
				if (!_pGE) return;

				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const Component::WorldMatrixComponent& _worldMatComp = a_worldMatArray[_i];
					const Component::ModelComponent& _modelComp = a_modelArray[_i];

					// モデル取得
					auto* _pModel = a_ctx.pServices->pResourceManager->Get(_modelComp.handle);
					if (!_pModel) continue;

					// 描画
					_pGE->RefDrawSubmitter()->SubmitGroundModel(
						*a_ctx.pWorld,
						_pModel,
						_worldMatComp.worldMat
					);
				}
			}
		);
	}
}
