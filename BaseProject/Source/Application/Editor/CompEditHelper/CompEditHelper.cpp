#include "CompEditHelper.h"

#include "../../../Engine/ECS/World/World.h"

#include "../../Components/Hierarchy/HierarchyComponent.h"
#include "../../Components/Resource/ModelComponent.h"

namespace App::Editor
{
	void App::Editor::CompEditHelper::SelectModelNode(
		Engine::ECS::CompEditContext& a_editContext,
		UINT& a_nodeNameHash, 
		UINT& a_nodeIndex
	)
	{
		auto* _pWorld = a_editContext.pWorld;

		// 親エンティティはヒエラルキーから取得する
		Engine::ECS::Entity _parentID = Engine::ECS::Limits::INVALID_ENTITY;
		if (_pWorld && a_editContext.entity != Engine::ECS::Limits::INVALID_ENTITY)
		{
			auto* _pHierarchy = _pWorld->RefData<HierarchyComponent>(a_editContext.entity);
			if (_pHierarchy) _parentID = _pHierarchy->parentID;
		}

		if (!_pWorld || _parentID == Engine::ECS::Limits::INVALID_ENTITY)
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: No parent via HierarchyComponent.");
			return;
		}

		// 親のモデルコンポーネントを取得
		auto* _pParentModelComp = _pWorld->RefData<ModelComponent>(_parentID);
		if (!_pParentModelComp)
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: ModelComponent not found on Parent.");
			return;
		}

		// リソースマネージャーから実際のモデルを取得
		const auto* _pParentModel = Engine::Resource::ResourceManager::Instance().Get(_pParentModelComp->handle);
		if (!_pParentModel)
		{
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "Warning: Model Resource is null.");
			return;
		}

		// ノード一覧の描画自体はエンジン側の共通ヘルパーに任せる
		Engine::Editor::EditorHelper::DrawModelNodeCombo(
			"Target Node",
			_pParentModel,
			a_nodeIndex,
			a_nodeNameHash
		);
	}

}