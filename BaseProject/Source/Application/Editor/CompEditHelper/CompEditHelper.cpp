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

		// モデルが管理する全ノード配列を取得
		const auto& _nodes = _pParentModel->GetOriginalNodeVec();

		// 現在選択されているノード名を表示用として取得
		std::string _currentNodeName = "None / Invalid";
		if (a_nodeIndex < _nodes.size())
		{
			_currentNodeName = _nodes[a_nodeIndex].name;
		}

		// ImGuiのコンボボックスで選択可能にする
		if (ImGui::BeginCombo("Target Node", _currentNodeName.c_str()))
		{
			for (size_t _i = 0; _i < _nodes.size(); ++_i)
			{
				bool _isSelected = (a_nodeIndex == _i);

				if (ImGui::Selectable(_nodes[_i].name.c_str(), _isSelected))
				{
					a_nodeNameHash = _nodes[_i].nodeNameHash;
					a_nodeIndex = static_cast<UINT>(_i);
				}

				if (_isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

}