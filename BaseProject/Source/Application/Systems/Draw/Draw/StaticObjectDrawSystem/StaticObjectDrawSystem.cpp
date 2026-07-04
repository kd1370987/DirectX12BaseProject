#include "StaticObjectDrawSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/MainEngine.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "../../../../../Engine/Graphics/GraphicEngine.h"

#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Transform/PreviousWorldMatrixComponent.h"
#include "Application/Components/Resource/ModelComponent.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/RenderContext/ShapeDraw/ShapeDraw.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Application/Components/Resource/AnimatorComponent.h"

void StaticObjectDrawSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const WorldMatrixComponent, const ModelComponent>(
		Engine::ECS::ESystemType::Draw,
		"StaticObjectDrawSystem",
		[&a_world]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const WorldMatrixComponent* a_worldMatArray,
			const ModelComponent* a_modelArray
			)
		{
			// グラフィックスエンジン取得
			auto* _pGE = Engine::MainEngine::Instance().RefGraphicsEngine();
			if (!_pGE) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const WorldMatrixComponent& _worldMatComp = a_worldMatArray[_i];
				const ModelComponent& _modelComp = a_modelArray[_i];

				// モデル取得
				auto* _model = Engine::Resource::ResourceManager::Instance().Get(_modelComp.handle);
				if (!_model) continue;

				// 描画
				_pGE->SubmitModel(
					a_world,
					_model,
					_worldMatComp.worldMat,
					_modelComp.colorScale,
					_modelComp.emissiveScale
				);
			}
		},
		Engine::ECS::Exclude<AnimatorComponent, PreviousWorldMatrixComponent>()
	);
}
