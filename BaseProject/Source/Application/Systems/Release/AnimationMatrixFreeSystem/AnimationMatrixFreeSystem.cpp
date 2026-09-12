#include "AnimationMatrixFreeSystem.h"
#include "Application/ECS/World/APPWorld.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../../Components/Resource/ModelComponent.h"
#include "../../../Components/Resource/AnimatorComponent.h"
#include "../../../Components/Resource/NodePoseComponent.h"
#include "../../../Components/Resource/SkeletonPoseComponent.h"

#include "../../../../Engine/MainEngine.h"
#include "../../../../Engine/Graphics/GraphicEngine.h"
#include "../../../../Engine/Graphics/MeshBufferAllocator/MeshBufferAllocator.h"

void AnimationMatrixFreeSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ReleaseTask<const ModelComponent, AnimatorComponent, NodePoseComponent, SkeletonPoseComponent>(
		Engine::ECS::ESystemType::Release,
		"AnimationMatrixFreeSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ReleaseTag* a_releaseTag,
			const ModelComponent* a_pModelArray,
			AnimatorComponent* a_animationArray,
			NodePoseComponent* a_nodeArray,
			SkeletonPoseComponent* a_poseArray
		)
		{
			// ハンドルの登録
			auto* _pGE = a_ctx.pServices->pMainEngine->RefGraphicsEngine();
			ENGINE_ERRLOG(_pGE, "メッシュ解放時にGraphicsEngineが存在しません");

			// メガバッファにアロケート
			auto* _pMeshBufferAllocator = _pGE->RefMeshBufferAllocator();
			ENGINE_ERRLOG(_pMeshBufferAllocator, "メッシュ解放時にメッシュバッファアロケーターが存在しません");

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ModelComponent& _modelComp = a_pModelArray[_i];
				AnimatorComponent& _animationComp = a_animationArray[_i];
				NodePoseComponent& _nodeComp = a_nodeArray[_i];
				SkeletonPoseComponent& _poseComp = a_poseArray[_i];

				auto& _nodePosePool = a_ctx.pWorld->GetResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
				auto& _boneMatPool = a_ctx.pWorld->GetResource<Engine::Pool::RangePool<Engine::Resource::BoneMatrix>>();

				_nodePosePool.FreeRange(_nodeComp.nodePoseHandle);
				_boneMatPool.FreeRange(_poseComp.skeletonPoseHandle);

				// アニメーション用頂点データの解放
				auto& _dynamicRaytracingData = a_ctx.pWorld->GetResource<Engine::Pool::ItemPool<Engine::Raytracing::DynamicRaytracingData>>();
				auto* _pAnimData = _dynamicRaytracingData.Ref(_animationComp.dynamicInstanceHandle);
				if (!_pAnimData) continue;

				// メッシュデータのハンドル解放
				for (auto& _data : _pAnimData->meshDataVec)
				{
					_data.instanceBLAS.Release();
					_pMeshBufferAllocator->AnimatedVertexFree(_data.animatedVertexHandle);
				}

				// ダイナミックデータの解放
				_dynamicRaytracingData.Remove(_animationComp.dynamicInstanceHandle);
				_animationComp = {};
				ENGINE_LOG("アニメーションデータの解放");
			}
		}
	);
}
