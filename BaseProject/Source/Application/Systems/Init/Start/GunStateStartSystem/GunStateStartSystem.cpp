#include "GunStateStartSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Character/Weapon/Gun/GunStateComponent.h"
#include "../../../../Components/Resource/ModelComponent.h"

void GunStateStartSystem::Init(Engine::ECS::World& a_world)
{
	a_world.StartTask<const ModelComponent,GunStateComponent>(
		Engine::ECS::ESystemType::Start,
		"GunStateStartSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			StartTag* a_tag,
			const ModelComponent* a_modelArray,
			GunStateComponent* a_gunStateArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ModelComponent& _modelComp = a_modelArray[_i];
				GunStateComponent& _gunComp = a_gunStateArray[_i];

				// 親のモデルを取得
				const auto* _pParentModel = a_ctx.pServices->pResourceManager->Get(_modelComp.handle);
				if (!_pParentModel) continue;

				// モデルのノードを検索し、ハッシュ一致するノードのインデックスを解決
				for (UINT _nodeIdx = 0; _nodeIdx < _pParentModel->GetOriginalNodeVec().size(); ++_nodeIdx)
				{
					const auto& _node = _pParentModel->GetOriginalNodeVec()[_nodeIdx];

					// 違うのならスキップ
					if (_node.nodeNameHash != _gunComp.nullPtrNodeHash) continue;
					_gunComp.nodeIndex = _nodeIdx;
				}
			}
		}
	);
}
