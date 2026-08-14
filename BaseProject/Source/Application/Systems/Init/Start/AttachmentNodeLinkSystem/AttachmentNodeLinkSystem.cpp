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
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
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
				auto* _pParentModleComp = a_ctx.pWorld->RefData<ModelComponent>(_parentID);
				if (!_pParentModleComp) continue;
				const auto* _pParentModel = a_ctx.pServices->pResourceManager->Get(_pParentModleComp->handle);
				if (!_pParentModel) continue;

				// 読み込みが終わっていないモデルは中身が空で返る。
				// ポインタは有効なので上のnullチェックでは弾けず、
				// このまま検索すると一致が見つからずノード番号が既定値のまま残る
				// (別のノードへ張り付く)。ここまで来るのはゲートの取りこぼし
				if (_pParentModel->GetOriginalNodeVec().empty())
				{
					ENGINE_WARNING("[Attachment] 親モデルがまだ空です。追従ノードを解決できません");
					continue;
				}

				// モデルのノードを検索し、ハッシュ一致するノードのインデックスを解決
				bool _isFound = false;
				for (UINT _nodeIdx = 0; _nodeIdx < _pParentModel->GetOriginalNodeVec().size(); ++_nodeIdx)
				{
					const auto& _node = _pParentModel->GetOriginalNodeVec()[_nodeIdx];

					// 違うのならスキップ
					if (_node.nodeNameHash != _followComp.targetNodeHash) continue;

					_followComp.targetNodeIdx = _nodeIdx;
					_isFound = true;
				}

				// 見つからないまま既定値(0)で追従すると、
				// 意図しないノードに張り付いたまま気づけない
				if (!_isFound)
				{
					ENGINE_WARNING("[Attachment] 追従ノードが見つかりません : hash=%u", _followComp.targetNodeHash);
				}
			}
		}
	);
}
