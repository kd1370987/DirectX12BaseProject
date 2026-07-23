#include "AnimationSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Resource/AnimatorComponent.h"
#include "Application/Components/Resource/NodePoseComponent.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

void AnimationSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ModelComponent, AnimatorComponent, NodePoseComponent>(
		Engine::ECS::ESystemType::Animation,
		"AnimationSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
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
				AnimatorComponent& _aniComp = a_animatorArray[_i];
				NodePoseComponent& _nodeComp = a_NodePoseArray[_i];

				// モデル取得
				const auto* _pModel = Engine::Resource::ResourceManager::Instance().Get(_modelComp.handle);
				if (!_pModel) continue;

				// アニメーション取得
				const auto* _pAni = Engine::Resource::ResourceManager::Instance().Get(_aniComp.animHandle);
				if (!_pAni) continue;

				// ノードポーズ行列配列取得
				auto& _nodePosePool = a_ctx.pWorld->GetResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
				auto _nodePoseVec = _nodePosePool.RefRange(_nodeComp.nodePoseHandle);
				if (_nodePoseVec.empty()) continue;

				// チャンネル適用前に、全ノードのlocalをモデルのバインドポーズで毎フレームリセットする。
				// これをしないと、クリップが触らないノード(ルート/アーマチュアの向き補正や
				// 腰オフセット等)がIdentityのまま残り、斜め上を向いて浮く原因になる。
				// モデル差し替え直後に旧クリップが残っていても、現モデルのバインドポーズに戻せる。
				{
					const auto& _nodes = _pModel->GetOriginalNodeVec();
					for (size_t _n = 0; _n < _nodePoseVec.size() && _n < _nodes.size(); ++_n)
					{
						_nodePoseVec[_n].local = _nodes[_n].localTransform;
					}
				}

				// すべてのアニメーションノードの行列保管を実行する
				for (size_t _j = 0; _j < _pAni->nodes.size(); ++_j)
				{
					UINT _idx = _pAni->nodes[_j].nodeOffset;

					// モデル差し替え直後などは、ステートマシンが旧モデル用の
					// アニメーションを指したままのことがある。
					// 範囲外のチャンネルは適用せずスキップする
					if (_idx >= _nodePoseVec.size()) continue;

					Engine::Animation::Interpolate(_pAni->nodes[_j], _aniComp.time, _nodePoseVec[_idx].local);
				}

				// アニメーションタイム進行
				_aniComp.time += a_ctx.dt * _aniComp.speed;

				if (_aniComp.time >= _pAni->maxLength)
				{
					if (_aniComp.isLoop != 0)
					{
						_aniComp.time = 0.0f;
					}
					else
					{
						_aniComp.time = _pAni->maxLength;
					}
				}
			}
		}
	);
}