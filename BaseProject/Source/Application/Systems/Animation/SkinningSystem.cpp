#include "SkinningSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Render/ModelComponent.h"
#include "Application/Components/Animation/NodePoseComponent.h"
#include "Application/Components/Animation/SkeletonPoseComponent.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
namespace App::System
{
	void SkinningSystem::Init(App::ECS::APPWorld& a_world)
	{
		// ノードポーズは読むだけなので const。
		// 書き込み扱いにすると、ワールド行列を組む CalcNodeSystem / AdditivePoseSystem との間に
		// 依存の辺が張られず、それより先に走って前フレームの行列でボーンを作ってしまう。
		// 自分のチャンクとプールの自分の範囲だけを書くので、チャンクを分けてワーカーで回す
		a_world.ActiveJobTask<const Component::ModelComponent, const Component::NodePoseComponent, Component::SkeletonPoseComponent>(
			Engine::ECS::ESystemType::Animation,
			"SkinningSystem",
			[](
				Engine::ECS::Chunk* /*a_pChunk*/,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ActiveTag* /*a_tags*/,
				const Component::ModelComponent* a_modelArray,
				const Component::NodePoseComponent* a_nodePoseArray,
				Component::SkeletonPoseComponent* a_skePoseArray
			)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const Component::ModelComponent& _modelComp = a_modelArray[_i];
					const Component::NodePoseComponent& _nodeComp = a_nodePoseArray[_i];
					Component::SkeletonPoseComponent& _skeComp = a_skePoseArray[_i];

					// モデル取得 : 引けないものだけ飛ばす(同じチャンクの残りは続ける)
					auto* _pModel = a_ctx.pServices->pResourceManager->Get(_modelComp.handle);
					if (!_pModel) continue;

					// 全ノード
					const auto& _dataNodes = _pModel->GetOriginalNodeVec();

					// 全スケルタルポーズを初期化
					auto& _boneMatPool = a_ctx.pWorld->RefResource<Engine::Pool::RangePool<Engine::Resource::BoneMatrix>>();
					auto _boneMatVec = _boneMatPool.RefRange(_skeComp.skeletonPoseHandle);
					for (auto& _mat : _boneMatVec)
					{
						_mat.mat = Math::Matrix::Identity();
					}

					// 全ノードポーズを再帰計算
					auto& _nodePosePool = a_ctx.pWorld->RefResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
					auto _nodePoseMatVec = _nodePosePool.RefRange(_nodeComp.nodePoseHandle);

					// ボーンノード
					for (auto& _nodeIdx : _pModel->GetBoneNodeVec())
					{
						// モデル差し替え直後などのサイズ不整合や、領域確保失敗(空span)の場合は
						// 落とさずスキップする
						if (_nodeIdx < 0 || static_cast<size_t>(_nodeIdx) >= _nodePoseMatVec.size()) continue;

						const auto& _dataNode = _dataNodes[_nodeIdx];
						if (_dataNode.boneIndex < 0 || static_cast<size_t>(_dataNode.boneIndex) >= _boneMatVec.size()) continue;

						Math::Matrix _nodeWorldMat = _nodePoseMatVec[_nodeIdx].world;
						Math::Matrix _invMat = _dataNodes[_nodeIdx].boneInverseWorldMatrix;
						_boneMatVec[_dataNode.boneIndex].mat = _invMat * _nodeWorldMat;
					}

				}
			}
		);
	}
}