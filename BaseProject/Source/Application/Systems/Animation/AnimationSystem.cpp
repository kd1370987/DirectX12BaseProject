#include "AnimationSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Render/ModelComponent.h"
#include "Application/Components/Animation/AnimatorComponent.h"
#include "Application/Components/Animation/NodePoseComponent.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace
{
	// レイヤーの更新
	void UpdateLayer(const Engine::ECS::SystemContext& a_ctx, const ModelComponent& a_modelComp,AnimatorLayer& a_layer,NodePoseComponent& a_nodeposeComp)
	{
		// モデル取得
		const auto* _pModel = a_ctx.pServices->pResourceManager->Get(a_modelComp.handle);
		if (!_pModel) return;

		// 今のステートのノード(再生するクリップ・速さ・ループ)。
		const auto* _pAnimator = a_ctx.pServices->pResourceManager->Get(a_layer.animatorHandle);
		if (!_pAnimator) return;
		const auto* _pNode = _pAnimator->GetStateNode(a_layer.currentStateHash);
		if (!_pNode) return;;

		// アニメーション取得
		const auto* _pAni = a_ctx.pServices->pResourceManager->Get(_pNode->playAnimData);
		if (!_pAni) return;

		// ノードポーズ行列配列取得
		auto& _nodePosePool = a_ctx.pWorld->GetResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
		auto _nodePoseVec = _nodePosePool.RefRange(a_nodeposeComp.nodePoseHandle);
		if (_nodePoseVec.empty()) return;

		// チャンネル適用前に、全ノードのlocalをモデルのバインドポーズで毎フレームリセット
		const auto& _nodes = _pModel->GetOriginalNodeVec();
		for (size_t _n = 0; _n < _nodePoseVec.size() && _n < _nodes.size(); ++_n)
		{
			_nodePoseVec[_n].local = _nodes[_n].localTransform;
		}

		// すべてのアニメーションノードの行列保管を実行する
		for (size_t _j = 0; _j < _pAni->nodes.size(); ++_j)
		{
			UINT _idx = _pAni->nodes[_j].nodeOffset;

			// 範囲外のチャンネルは適用せずスキップ
			if (_idx >= _nodePoseVec.size()) continue;

			// ノード計算
			Engine::Animation::Interpolate(_pAni->nodes[_j], a_layer.clipTime, _nodePoseVec[_idx].local);
		}

		// アニメーションタイム進行
		a_layer.clipTime += a_ctx.dt * _pNode->speed;

		if (a_layer.clipTime >= _pAni->maxLength)
		{
			if (_pNode->isLoop)
			{
				a_layer.clipTime = 0.0f;
			}
			else
			{
				a_layer.clipTime = _pAni->maxLength;
			}
		}
	}
}

void AnimationSystem::Init(App::ECS::APPWorld& a_world)
{
	// 自分のチャンクとプールの自分の範囲だけを書くので、チャンクを分けてワーカーで回す
	a_world.ActiveJobTask<const ModelComponent, AnimatorComponent, NodePoseComponent>(
		Engine::ECS::ESystemType::Animation,
		"AnimationSystem",
		[](
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const ModelComponent* a_modelArray,
			AnimatorComponent* a_animatorArray,
			NodePoseComponent* a_NodePoseArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ModelComponent& _modelComp = a_modelArray[_i];
				AnimatorComponent& _animatorComp = a_animatorArray[_i];
				NodePoseComponent& _nodeComp = a_NodePoseArray[_i];

				// レイヤー更新
				UpdateLayer(a_ctx, _modelComp, _animatorComp.baseLayer, _nodeComp);

				// アニメーションレイヤリングするのなら
				if(_animatorComp.isLayering)
				{
					//UpdateLayer(a_ctx, _modelComp, _animatorComp.upperLayer, _nodeComp);
				}
			}
		}
	)
	;
}