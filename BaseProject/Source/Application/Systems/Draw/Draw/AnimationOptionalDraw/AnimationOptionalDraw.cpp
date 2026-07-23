#include "AnimationOptionalDraw.h"

#include "Engine/ECS/World/World.h"
#include "Engine/MainEngine.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "../../../../../Engine/Graphics/GraphicEngine.h"

#include "Application/Components/Resource/SkeletonPoseComponent.h"
#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/AnimatorComponent.h"
#include "Application/Components/Resource/NodePoseComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"

void AnimationOptionalDrawSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<
		const WorldMatrixComponent,
		const ModelComponent, 
		const SkeletonPoseComponent,
		const AnimatorComponent, 
		const NodePoseComponent
	>(
		Engine::ECS::ESystemType::Draw,
		"AnimationOptionalDrawSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const WorldMatrixComponent* a_matArray,
			const ModelComponent* a_modelArray,
			const SkeletonPoseComponent* a_skeArray,
			const AnimatorComponent* a_aniArray,
			const NodePoseComponent* a_nodePoseArray
			)
		{
			// グラフィックスエンジン取得
			auto* _pGE = a_ctx.pServices->pMainEngine->RefGraphicsEngine();
			if (!_pGE) return;
		
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const WorldMatrixComponent& _matComp = a_matArray[_i];
				const ModelComponent& _modelComp = a_modelArray[_i];
				const SkeletonPoseComponent& _skeComp = a_skeArray[_i];
				const NodePoseComponent& _nodePoseComp = a_nodePoseArray[_i];
				const AnimatorComponent& _animComp = a_aniArray[_i];

				// モデル取得
				auto* _model = a_ctx.pServices->pResourceManager->Get(_modelComp.handle);
				if (!_model) continue;

				// 描画
				_pGE->SubmitModel(
					*a_ctx.pWorld,
					_model,
					_matComp.worldMat,
					_matComp.worldMat,
					_skeComp.skeletonPoseHandle,
					_nodePoseComp.nodePoseHandle,
					_animComp.dynamicInstanceHandle
				);
			}
		}); 
}