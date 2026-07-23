#include "AnimationModelStartSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../../../Components/Resource/ModelComponent.h"
#include "../../../../Components/Resource/AnimatorComponent.h"
#include "../../../../Components/Resource/NodePoseComponent.h"
#include "../../../../Components/Resource/SkeletonPoseComponent.h"

void AnimationModelStartSystem::Init(Engine::ECS::World& a_world)
{
	a_world.StartTask<const ModelComponent,AnimatorComponent,NodePoseComponent,SkeletonPoseComponent>(
		Engine::ECS::ESystemType::Awake,
		"AnimationModelStartSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			StartTag* a_startTag,
			const ModelComponent* a_pModelArray, 
			AnimatorComponent* a_animationArray,
			NodePoseComponent* a_nodeArray, 
			SkeletonPoseComponent* a_poseArray
			
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ModelComponent& _modelComp = a_pModelArray[_i];
				AnimatorComponent& _animationComp = a_animationArray[_i];
				NodePoseComponent& _nodeComp = a_nodeArray[_i];
				SkeletonPoseComponent& _poseComp = a_poseArray[_i];

				// モデル取得
				auto* _pModel = Engine::Resource::ResourceManager::Instance().Get(_modelComp.handle);
				if (!_pModel) continue;

				// アニメーター初期化
				_animationComp.clipID = 0;
				_animationComp.time = 0.0f;
				_animationComp.speed = 30.0f;
				_animationComp.isLoop = true;

				// ノードポーズ行列領域確保
				auto& _nodePosePool = a_ctx.pWorld->GetResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
	
				// モデルのアニメーションから最大ノードを持つものを取得
				UINT _totalNodeCount = static_cast<UINT>(_pModel->GetOriginalNodeVec().size());
				_nodeComp.nodePoseHandle = _nodePosePool.AllocateRange(_totalNodeCount);

				// ノードポーズ初期化：localはモデルのバインドポーズで初期化する。
				// Identityにすると、アニメが触らないノードのバインド変換が消えて
				// 斜め上を向いて浮く原因になる。
				{
					const auto& _nodes = _pModel->GetOriginalNodeVec();
					auto _nodePoseVec = _nodePosePool.RefRange(_nodeComp.nodePoseHandle);
					for (size_t _n = 0; _n < _nodePoseVec.size(); ++_n)
					{
						_nodePoseVec[_n].local = (_n < _nodes.size()) ? _nodes[_n].localTransform : DXSM::Matrix::Identity;
						_nodePoseVec[_n].world = DXSM::Matrix::Identity;
					}
				}

				// ボーン行列領域確保
				auto& _boneMatPool = a_ctx.pWorld->GetResource<Engine::Pool::RangePool<Engine::Resource::BoneMatrix>>();
				size_t _boneNodeCount = _pModel->GetBoneNodeVec().size();
				_poseComp.skeletonPoseHandle = _boneMatPool.AllocateRange(static_cast<uint32_t>(_boneNodeCount));

				// スケルタルポーズ初期化
				for (auto& _mat : _boneMatPool.RefRange(_poseComp.skeletonPoseHandle))
				{
					_mat.mat = DXSM::Matrix::Identity;
				}

				// BLASインスタンス確保
				auto& _dynamicInstancePool = 
					a_ctx.pWorld->GetResource<Engine::Pool::ItemPool<Engine::Raytracing::DynamicRaytracingData>>();

				// 空で生成
				Engine::Raytracing::DynamicRaytracingData _resource = {};
				_animationComp.dynamicInstanceHandle = _dynamicInstancePool.Add(std::move(_resource));

				// GPU処理のため遅延生成用命令
				auto& _initRequestVec = a_ctx.pWorld->GetResource<std::vector<Engine::Raytracing::DynamicRaytracingInitRequest>>();
				Engine::Raytracing::DynamicRaytracingInitRequest _req = {};
				_req.dynamicInstanceHandle = _animationComp.dynamicInstanceHandle;
				_req.modelHandle = _modelComp.handle;
				_initRequestVec.push_back(_req);
			}
		}
	);
}
