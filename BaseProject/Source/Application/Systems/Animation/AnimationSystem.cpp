#include "AnimationSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Render/ModelComponent.h"
#include "Application/Components/Animation/AnimatorComponent.h"
#include "Application/Components/Animation/UpperAnimatorComponent.h"
#include "Application/Components/Animation/NodePoseComponent.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace App::System
{
	namespace
	{
		// レイヤーの今のステートで再生するもの
		struct LayerClip
		{
			const Engine::Resource::AnimatorNode* pNode = nullptr;		// 速さ・ループ
			const Engine::Resource::AnimationData* pAni = nullptr;		// クリップ
		};

		//----------------------------------------------------------------------------------
		// レイヤーの今のステートのノードとクリップを引く。どちらかが引けなければ false
		//----------------------------------------------------------------------------------
		bool FindLayerClip(const Engine::ECS::SystemContext& a_ctx, const Component::AnimatorLayer& a_layer, LayerClip& a_rOut)
		{
			const auto* _pAnimator = a_ctx.pServices->pResourceManager->Get(a_layer.animatorHandle);
			if (!_pAnimator) return false;

			a_rOut.pNode = _pAnimator->GetStateNode(a_layer.currentStateHash);
			if (!a_rOut.pNode) return false;

			a_rOut.pAni = a_ctx.pServices->pResourceManager->Get(a_rOut.pNode->playAnimData);
			return a_rOut.pAni != nullptr;
		}

		//----------------------------------------------------------------------------------
		// クリップのチャンネルをローカル行列の配列へ書く
		//----------------------------------------------------------------------------------
		template<class LocalFunc>
		void ApplyClip(const Engine::Resource::AnimationData& a_ani, float a_clipTime, size_t a_nodeCount, LocalFunc a_refLocal)
		{
			for (size_t _j = 0; _j < a_ani.nodes.size(); ++_j)
			{
				UINT _idx = a_ani.nodes[_j].nodeOffset;

				// モデル差し替え直後などは、ステートマシンが旧モデル用の
				// アニメーションを指したままのことがある。範囲外のチャンネルは適用せずスキップする
				if (_idx >= a_nodeCount) continue;

				Engine::Graphics::Animation::Interpolate(a_ani.nodes[_j], a_clipTime, a_refLocal(_idx));
			}
		}

		//----------------------------------------------------------------------------------
		// クリップの再生位置を進める
		//----------------------------------------------------------------------------------
		void AdvanceClipTime(const Engine::ECS::SystemContext& a_ctx, const LayerClip& a_clip, Component::AnimatorLayer& a_layer)
		{
			a_layer.clipTime += a_ctx.dt * a_clip.pNode->speed;

			if (a_layer.clipTime >= a_clip.pAni->maxLength)
			{
				if (a_clip.pNode->isLoop)
				{
					a_layer.clipTime = 0.0f;
				}
				else
				{
					a_layer.clipTime = a_clip.pAni->maxLength;
				}
			}
		}

		//----------------------------------------------------------------------------------
		// 基本レイヤー : バインドポーズへ戻してから、クリップで全身を書く
		//----------------------------------------------------------------------------------
		void UpdateBaseLayer(const Engine::ECS::SystemContext& a_ctx, const Component::ModelComponent& a_modelComp, Component::AnimatorLayer& a_layer, Component::NodePoseComponent& a_nodeposeComp)
		{
			// モデル取得
			const auto* _pModel = a_ctx.pServices->pResourceManager->Get(a_modelComp.handle);
			if (!_pModel) return;

			// 今のステートのクリップ(速さ・ループはノードから)
			LayerClip _clip = {};
			if (!FindLayerClip(a_ctx, a_layer, _clip)) return;

			// ノードポーズ行列配列取得
			auto& _nodePosePool = a_ctx.pWorld->RefResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
			auto _nodePoseVec = _nodePosePool.RefRange(a_nodeposeComp.nodePoseHandle);
			if (_nodePoseVec.empty()) return;

			// チャンネル適用前に、全ノードのlocalをモデルのバインドポーズで毎フレームリセットする。
			// これをしないと、クリップが触らないノード(ルート/アーマチュアの向き補正や
			// 腰オフセット等)がIdentityのまま残り、斜め上を向いて浮く原因になる。
			const auto& _nodes = _pModel->GetOriginalNodeVec();
			for (size_t _n = 0; _n < _nodePoseVec.size() && _n < _nodes.size(); ++_n)
			{
				_nodePoseVec[_n].local = _nodes[_n].localTransform;
			}

			// すべてのアニメーションノードの行列保管を実行する
			ApplyClip(*_clip.pAni, a_layer.clipTime, _nodePoseVec.size(),
				[&_nodePoseVec](size_t a_idx) -> Math::Matrix& { return _nodePoseVec[a_idx].local; });

			// アニメーションタイム進行
			AdvanceClipTime(a_ctx, _clip, a_layer);
		}

		//----------------------------------------------------------------------------------
		// 上に重ねるレイヤー : 基本レイヤーが書いた配列を、ボーンレイヤーに載っているノードだけ上書きする
		//----------------------------------------------------------------------------------
		void UpdateUpperLayer(const Engine::ECS::SystemContext& a_ctx, const Component::ModelComponent& a_modelComp, Component::UpperAnimatorComponent& a_upperComp, Component::NodePoseComponent& a_nodeposeComp)
		{
			Component::AnimatorLayer& _layer = a_upperComp.layer;

			// モデル取得
			const auto* _pModel = a_ctx.pServices->pResourceManager->Get(a_modelComp.handle);
			if (!_pModel) return;

			// 今のステートのクリップ
			LayerClip _clip = {};
			if (!FindLayerClip(a_ctx, _layer, _clip)) return;

			// ノードポーズ行列配列取得
			auto& _nodePosePool = a_ctx.pWorld->RefResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
			auto _nodePoseVec = _nodePosePool.RefRange(a_nodeposeComp.nodePoseHandle);
			if (_nodePoseVec.empty()) return;

			// ボーンレイヤー : 0 なら全身。
			// 選んでいるのにモデルに無い(モデル差し替え直後など)ときは、全身にかけずに何もしない
			const Engine::Resource::BoneMask* _pMask = nullptr;
			if (_layer.boneMaskHash != 0)
			{
				_pMask = _pModel->FindBoneMask(_layer.boneMaskHash);
			}
			const bool _hasTarget = (_layer.boneMaskHash == 0) || (_pMask != nullptr);

			// 効きが0でも再生位置は進める(効きを戻したときに止まっていた所から始まらないように)
			const float _layerWeight = std::clamp(a_upperComp.weight, 0.0f, 1.0f);
			if (_hasTarget && _layerWeight > 0.0f)
			{
				const auto& _nodes = _pModel->GetOriginalNodeVec();
				const size_t _nodeCount = std::min(_nodePoseVec.size(), _nodes.size());

				// このレイヤーだけのポーズを作る。
				// 基本レイヤーと同じく、バインドポーズから始めてクリップのチャンネルを書く
				// (重み1なら、このクリップを基本レイヤーで再生したのと同じ見た目になる)。
				// ワーカーで回るので、作業用の配列はスレッドごとに持って使い回す
				thread_local std::vector<Math::Matrix> t_layerLocalVec;
				t_layerLocalVec.resize(_nodeCount);
				for (size_t _n = 0; _n < _nodeCount; ++_n)
				{
					t_layerLocalVec[_n] = _nodes[_n].localTransform;
				}
				ApplyClip(*_clip.pAni, _layer.clipTime, _nodeCount,
					[](size_t a_idx) -> Math::Matrix& { return t_layerLocalVec[a_idx]; });

				// 基本レイヤーの結果へ重ねる(重み1なら上書き、途中なら TRS で補間)
				auto _Overwrite = [&](size_t a_idx, float a_boneWeight)
					{
						const float _weight = a_boneWeight * _layerWeight;
						if (_weight <= 0.0f) return;
						_nodePoseVec[a_idx].local = Engine::Graphics::Animation::BlendLocalMatrix(_nodePoseVec[a_idx].local, t_layerLocalVec[a_idx], _weight);
					};

				if (_pMask)
				{
					for (const auto& _bone : _pMask->bones)
					{
						if (_bone.nodeIndex >= _nodeCount) continue;
						_Overwrite(_bone.nodeIndex, _bone.weight);
					}
				}
				else
				{
					for (size_t _n = 0; _n < _nodeCount; ++_n)
					{
						_Overwrite(_n, 1.0f);
					}
				}
			}

			// アニメーションタイム進行
			AdvanceClipTime(a_ctx, _clip, _layer);
		}
	}

	void AnimationSystem::Init(App::ECS::APPWorld& a_world)
	{
		// 自分のチャンクとプールの自分の範囲だけを書くので、チャンクを分けてワーカーで回す
		a_world.ActiveJobTask<const Component::ModelComponent, Component::AnimatorComponent, Component::NodePoseComponent>(
			Engine::ECS::ESystemType::Animation,
			"AnimationSystem",
			[](
				Engine::ECS::Chunk* /*a_pChunk*/,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ActiveTag* /*a_tags*/,
				const Component::ModelComponent* a_modelArray,
				Component::AnimatorComponent* a_animatorArray,
				Component::NodePoseComponent* a_NodePoseArray
			)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					// 基本レイヤー : 全身
					UpdateBaseLayer(a_ctx, a_modelArray[_i], a_animatorArray[_i].baseLayer, a_NodePoseArray[_i]);
				}
			}
		);

		//--------------------------------------------------------------------------------------
		// アニメーションレイヤリング : 基本レイヤーが書いた配列へ、上に重ねるレイヤーを書く
		//--------------------------------------------------------------------------------------
		a_world.ActiveJobTask<const Component::ModelComponent, Component::UpperAnimatorComponent, Component::NodePoseComponent>(
			Engine::ECS::ESystemType::Animation,
			"UpperAnimationSystem",
			[](
				Engine::ECS::Chunk* /*a_pChunk*/,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ActiveTag* /*a_tags*/,
				const Component::ModelComponent* a_modelArray,
				Component::UpperAnimatorComponent* a_upperArray,
				Component::NodePoseComponent* a_NodePoseArray
			)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					UpdateUpperLayer(a_ctx, a_modelArray[_i], a_upperArray[_i], a_NodePoseArray[_i]);
				}
			}
		)
		// 順序 : 基本レイヤーを書いた後に上書きする(書き手同士なので向きを決めておく)
		.After("AnimationSystem");
	}
}
