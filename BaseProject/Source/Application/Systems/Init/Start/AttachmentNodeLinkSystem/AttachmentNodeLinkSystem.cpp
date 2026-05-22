#include "AttachmentNodeLinkSystem.h"

#include "Engine/ECS/World/World.h"
#include "../../../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../../../Components/Hierarchy/ExoskeletonAttachementComponent.h"
#include "../../../../Components/Resource/ModelComponent.h"

void AttachmentNodeLinkSystem::Init(Engine::ECS::World& a_world)
{
	a_world.StartTask<ExoskeletonAttachmentComponent>(
		Engine::ECS::ESystemType::Start,
		"AttachmentNodeLinkSystem",
		[&a_world](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			StartTag* a_tag,
			ExoskeletonAttachmentComponent* a_attacheArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ExoskeletonAttachmentComponent& _exoComp = a_attacheArray[_i];
				
				// 親のモデルを取得
				auto* _pParentModleComp = a_world.RefData<ModelComponent>(_exoComp.parentID);
				if (!_pParentModleComp) continue;
				const auto* _pParentModel = Engine::Resource::ResourceManager::Instance().Get(_pParentModleComp->handle);
				if (!_pParentModel) continue;

				// モデルのノードを検索
				for (UINT _i = 0; _i < _pParentModel->GetOriginalNodeVec().size(); ++_i)
				{
					auto& _node = _pParentModel->GetOriginalNodeVec()[_i];

					// 違うのならスキップ
					if (_node.nodeNameHash != _exoComp.targetNodeHash) continue;

					_exoComp.targetNodeIdx = _i;
				}
			}
		}
	);
}
