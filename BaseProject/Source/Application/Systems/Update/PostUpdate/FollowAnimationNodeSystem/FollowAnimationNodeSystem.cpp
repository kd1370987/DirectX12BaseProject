#include "FollowAnimationNodeSystem.h"
#include "Application/ECS/World/World.h"

#include "../../../../Components/Hierarchy/FollowAnimationNodeComponent.h"
#include "../../../../Components/Hierarchy/HierarchyComponent.h"
#include "../../../../Components/Resource/NodePoseComponent.h"

#include "Application/Components/Transform/LocalTransformComponent.h"

void FollowAnimationNodeSystem::Init(App::ECS::World& a_world)
{
	a_world.ActiveTask<const FollowAnimationNodeComponent, const HierarchyComponent, LocalTransformComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		"FollowAnimationNodeSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const FollowAnimationNodeComponent* a_followArray,
			const HierarchyComponent* a_hierarchyArray,
			LocalTransformComponent* a_transArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const FollowAnimationNodeComponent& _followComp = a_followArray[_i];
				const HierarchyComponent& _hierarchyComp = a_hierarchyArray[_i];
				LocalTransformComponent& _trsComp = a_transArray[_i];

				// 親エンティティはヒエラルキーから取得する
				Engine::ECS::Entity _parentID = _hierarchyComp.parentID;
				if (_parentID == Engine::ECS::Limits::INVALID_ENTITY) continue;

				// 親のノードポーズ配列を取得
				auto* _pNodePoseComp = a_ctx.pWorld->RefData<NodePoseComponent>(_parentID);
				if (!_pNodePoseComp) continue;

				// ノード行列配列取得
				auto& _nodePosePool = a_ctx.pWorld->GetResource<Engine::Pool::RangePool<Engine::Resource::NodePoseMatrix>>();
				auto _nodePoseVec = _nodePosePool.RefRange(_pNodePoseComp->nodePoseHandle);
				if (_nodePoseVec.empty()) continue;
				if (_followComp.targetNodeIdx >= _nodePoseVec.size()) continue;

				// 対象ノードのローカル行列(親モデル空間)を取得。
				// 親エンティティのワールドは後段の CommitHierarchyWorldMatrixSystem が
				// world = local * parentWorld として掛けるため、ここでは掛けない。
				const Math::Matrix _nodeMat = _nodePoseVec[_followComp.targetNodeIdx].world;

				// ノード基準のオフセット行列(回転 → スケール → 平行移動)
				const Math::Matrix _offsetMat =
					Math::Matrix::CreateFromQuaternion(_followComp.offsetRotation) *
					Math::Matrix::CreateScale(_followComp.offsetScale) *
					Math::Matrix::CreateTranslation(_followComp.offsetPosition);

				// ノードのローカルに、ノード基準のオフセットを適用したローカル行列
				const Math::Matrix _finalLocalMat = _offsetMat * _nodeMat;

				// 行列から位置・回転・スケールを抽出して書き戻す
				_finalLocalMat.Decompose(_trsComp.scale, _trsComp.quat, _trsComp.pos);

				_trsComp.isDirty = true;
			}
		}
	);
}
