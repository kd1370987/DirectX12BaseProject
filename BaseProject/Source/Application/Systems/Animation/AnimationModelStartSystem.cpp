#include "AnimationModelStartSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "Application/Components/Render/ModelComponent.h"
#include "Application/Components/Animation/AnimatorComponent.h"
#include "Application/Components/Render/DynamicRaytracingComponent.h"
#include "Application/Components/Animation/NodePoseComponent.h"
#include "Application/Components/Animation/SkeletonPoseComponent.h"

namespace App::System
{
	void AnimationModelStartSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.StartTask<const Component::ModelComponent, const Component::AnimatorComponent, Component::NodePoseComponent, Component::SkeletonPoseComponent, Component::DynamicRaytracingComponent>(
			// StartTag を見るので Start フェーズで回す。
			// アニメーターとポーズ領域を確保する側なので、これを使う
			// AttachmentNodeLinkSystem / AdditivePoseLinkSystem より先に登録しておくこと
			// (互いに読み書きが噛み合わないため、順序は登録順で決まる)
			Engine::ECS::ESystemType::Start,
			"AnimationModelStartSystem",
			[](
				Engine::ECS::Chunk* a_pChunk,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::StartTag* a_startTag,
				const Component::ModelComponent* a_pModelArray, 
				const Component::AnimatorComponent*,		// アニメーションするモデルの目印
				Component::NodePoseComponent* a_nodeArray,
				Component::SkeletonPoseComponent* a_poseArray,
				Component::DynamicRaytracingComponent* a_rayArray
			)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const Component::ModelComponent& _modelComp = a_pModelArray[_i];
					Component::DynamicRaytracingComponent& _rayComp = a_rayArray[_i];
					Component::NodePoseComponent& _nodeComp = a_nodeArray[_i];
					Component::SkeletonPoseComponent& _poseComp = a_poseArray[_i];

					// モデル取得
					auto* _pModel = a_ctx.pServices->pResourceManager->Get(_modelComp.handle);
					if (!_pModel) continue;

					// ノードポーズ行列領域確保
					auto& _nodePosePool = a_ctx.pWorld->RefResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
	
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
							_nodePoseVec[_n].local = (_n < _nodes.size()) ? Math::Matrix(_nodes[_n].localTransform) : Math::Matrix::Identity();
							_nodePoseVec[_n].world = Math::Matrix::Identity();
						}
					}

					// ボーン行列領域確保
					auto& _boneMatPool = a_ctx.pWorld->RefResource<Engine::Pool::RangePool<Engine::Resource::BoneMatrix>>();
					size_t _boneNodeCount = _pModel->GetBoneNodeVec().size();
					_poseComp.skeletonPoseHandle = _boneMatPool.AllocateRange(static_cast<uint32_t>(_boneNodeCount));

					// スケルタルポーズ初期化
					for (auto& _mat : _boneMatPool.RefRange(_poseComp.skeletonPoseHandle))
					{
						_mat.mat = Math::Matrix::Identity();
					}

					// BLASインスタンス確保
					auto& _dynamicInstancePool = 
						a_ctx.pWorld->RefResource<Engine::Pool::ItemPool<Engine::Graphics::Raytracing::DynamicRaytracingData>>();

					// 空で生成
					Engine::Graphics::Raytracing::DynamicRaytracingData _resource = {};
					_rayComp.dynamicInstanceHandle = _dynamicInstancePool.Add(std::move(_resource));

					// GPU処理のため遅延生成用命令
					auto& _initRequestVec = a_ctx.pWorld->RefResource<std::vector<Engine::Graphics::Raytracing::DynamicRaytracingInitRequest>>();
					Engine::Graphics::Raytracing::DynamicRaytracingInitRequest _req = {};
					_req.dynamicInstanceHandle = _rayComp.dynamicInstanceHandle;
					_req.modelHandle = _modelComp.handle;
					_initRequestVec.push_back(_req);
				}
			}
		);
	}
}
