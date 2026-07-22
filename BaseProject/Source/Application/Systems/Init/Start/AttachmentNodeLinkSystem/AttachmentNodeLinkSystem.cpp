#include "AttachmentNodeLinkSystem.h"

#include "Engine/ECS/World/World.h"
#include "../../../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../../../Components/Hierarchy/FollowAnimationNodeComponent.h"
#include "../../../../Components/Hierarchy/HierarchyComponent.h"
#include "../../../../Components/Resource/ModelComponent.h"

void AttachmentNodeLinkSystem::Init(Engine::ECS::World& a_world)
{
	a_world.StartTask<FollowAnimationNodeComponent, const HierarchyComponent>(
		Engine::ECS::ESystemType::Start,
		"AttachmentNodeLinkSystem",
		[&a_world](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			StartTag* a_tag,
			FollowAnimationNodeComponent* a_followArray,
			const HierarchyComponent* a_hierarchyArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				FollowAnimationNodeComponent& _followComp = a_followArray[_i];
				const HierarchyComponent& _hierarchyComp = a_hierarchyArray[_i];

				// 親エンティティはヒエラルキーから取得する
				Engine::ECS::Entity _parentID = _hierarchyComp.parentID;
				if (_parentID == Engine::ECS::Limits::INVALID_ENTITY) continue;

				// 親のモデルを取得
				auto* _pParentModleComp = a_world.RefData<ModelComponent>(_parentID);
				if (!_pParentModleComp) continue;
				const auto* _pParentModel = Engine::Resource::ResourceManager::Instance().Get(_pParentModleComp->handle);
				if (!_pParentModel) continue;

				// モデルのノードを検索し、ハッシュ一致するノードのインデックスを解決
				for (UINT _nodeIdx = 0; _nodeIdx < _pParentModel->GetOriginalNodeVec().size(); ++_nodeIdx)
				{
					const auto& _node = _pParentModel->GetOriginalNodeVec()[_nodeIdx];

					// 違うのならスキップ
					if (_node.nodeNameHash != _followComp.targetNodeHash) continue;

					_followComp.targetNodeIdx = _nodeIdx;
				}
			}
		}
	);
}
