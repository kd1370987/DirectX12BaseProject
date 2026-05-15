#include "AnimationOptionalDraw.h"

#include "Engine/ECS/World/World.h"
#include "Engine/MainEngine.h"

#include "Application/Components/Resource/SkeletonPoseComponent.h"
#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/AnimatorComponent.h"
#include "Application/Components/Resource/NodePoseComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "../../../../../Engine/Animation/AnimationMatrixManager/AnimationMatrixManager.h"
//#include "Engine/Resource/Manager/ModelManager/ModelManager.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"

void AnimationOptionalDrawSystem::Init(Engine::ECS::World& a_world)
{
	//a_world.RegisterTask<const WorldMatrixComponent, const ModelComponent, const SkeletonPoseComponent, const AnimatorComponent, const NodePoseComponent>(
	a_world.ActiveTask<const WorldMatrixComponent, const ModelComponent, const SkeletonPoseComponent, const AnimatorComponent, const NodePoseComponent>(
		Engine::ECS::ESystemType::Draw,
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const WorldMatrixComponent* a_matArray,
			const ModelComponent* a_modelArray,
			const SkeletonPoseComponent* a_skeArray,
			const AnimatorComponent* a_aniArray,
			const NodePoseComponent* a_nodePoseArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const WorldMatrixComponent& _matComp = a_matArray[_i];
				const ModelComponent& _modelComp = a_modelArray[_i];
				const SkeletonPoseComponent& _skeComp = a_skeArray[_i];
				const AnimatorComponent& _aniComp = a_aniArray[_i];
				const NodePoseComponent& _nodePoseComp = a_nodePoseArray[_i];

				// 描画アイテム
				Engine::Graphics::DrawItem _item = {};
				_item.worldMat = _matComp.worldMat;
				_item.colorScale = _modelComp.colorScale;
				_item.emissiveScale = _modelComp.emissiveScale;

				// モデル取得
				auto* _model = Engine::Resource::ResourceManager::Instance().Get(_modelComp.handle);;
				if (!_model) return;

				// 行列取得
				// ボーン ノード
				auto* _boneMatVec = Engine::Animation::AnimationMatrixManager::Instance().AccessBoneMatVec(_skeComp.boneRange);
				auto* _nodeMatVec = Engine::Animation::AnimationMatrixManager::Instance().AccessNodePoseVec(_nodePoseComp.nodeRange);

				// 全ノード
				auto& _dataNodes = _model->GetOriginalNodeVec();

				// ボーンノード	
				_item.pBoneMatrices = _boneMatVec;
				_item.boneRange = _skeComp.boneRange;


				// 描画ノード
				for (auto& _nodeIdx : _model->GetDrawNodeVec())
				{
					for (auto& _meshIdx : _dataNodes[_nodeIdx].meshIndices)
					{
						// 描画メッシュ取得
						_item.pMesh = _model->GetSPMeshVec()[_meshIdx].get();
						if (!_item.pMesh) continue;

						// ワールド行列
						DXSM::Matrix _nodeTransMat(_nodeMatVec[_nodeIdx].world);
						DXSM::Matrix _worldMat(_matComp.worldMat);
						DXSM::Matrix _mat = _nodeTransMat * _worldMat;
						_item.worldMat = _mat;

						for (UINT _subIdx = 0; _subIdx < _item.pMesh->GetSubsets().size(); ++_subIdx)
						{
							// マテリアルセット
							if (_item.pMesh->GetSubsets()[_subIdx].faceCount == 0) continue;
							_item.pMaterial = _model->GetMaterialVec()[_item.pMesh->GetSubsets()[_subIdx].materialNumber].get();
							_item.subIdx = _subIdx;

							// 描画アイテムキューに送信

							// アルファモードによって描画先を変える
							Engine::Resource::Alpha _mode = _model->GetMaterialVec()[_item.pMesh->GetSubsets()[_subIdx].materialNumber]->alphaMode;

							auto* _pRCT = Engine::MainEngine::Instance().RefRenderContext();

							switch (_mode)
							{
							case Engine::Resource::Alpha::Opaque:
								_pRCT->AddItem(RenderQueueType::AnimationOpaque, _item);
								break;
							case Engine::Resource::Alpha::Mask:
								_pRCT->AddItem(RenderQueueType::AnimationOpaque, _item);
								break;
							case Engine::Resource::Alpha::Blend:
								_pRCT->AddItem(RenderQueueType::AnimationTransparent, _item);
								break;
							default:
								break;
							}
						}
					}
				}
			}
		}
	);
}