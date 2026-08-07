#include "GameObjectHierarchyPanel.h"

#include "../../../Scene/SceneManager/SceneManager.h"
#include "../../../GameObject/GameObjectManager/GameObjectManager.h"
#include "../../../GameObject/BaseObject/BaseObject.h"
#include "../../../GameObject/ObjectMetaRegistry/ObjectMetaRegistry.h"

namespace Engine::Editor
{
	void GameObjectHierarchyPanel::OnDrawImGui(EditorContext& a_editContext)
	{
		// このタブを見ている間はインスペクターをGameモードにする
		// (既存のHierarchyPanel/AssetDataBasePanelと同じ流儀。表示中のタブがモードを決める)
		a_editContext.eInspectorType = EInspectorType::Game;

		// 現在のシーンのオブジェクトマネージャーを取得
		auto* _pManager = Engine::Scene::SceneManager::Instance().RefGameObjectManager();
		if (!_pManager)
		{
			ImGui::TextDisabled("No GameObjectManager");
			a_editContext.pGameObject = nullptr;
			return;
		}

		// ------------------------------------------------------------------
		// AddObject : クラスメタマネージャーに登録済みのクラスを選んでシーンへ追加する
		// ------------------------------------------------------------------
		if (ImGui::Button("AddObject"))
		{
			ImGui::OpenPopup("AddObjectPopup");
		}
		if (ImGui::BeginPopup("AddObjectPopup"))
		{
			ImGui::TextDisabled("Select Class");
			ImGui::Separator();

			const auto& _allMeta = GameObject::ObjectMetaRegistry::Instance().GetAllMeta();
			if (_allMeta.empty())
			{
				ImGui::TextDisabled("No registered class");
			}
			else
			{
				// タイプインデックス順に並べて表示(map は順不同のため一旦ソート)
				std::vector<GameObject::ObjectTypeID> _ids;
				_ids.reserve(_allMeta.size());
				for (const auto& [_id, _meta] : _allMeta) _ids.push_back(_id);
				std::sort(_ids.begin(), _ids.end());

				for (GameObject::ObjectTypeID _id : _ids)
				{
					const auto& _meta = _allMeta.at(_id);
					std::string _label = _meta.name + "##addobj" + std::to_string(_id);
					if (ImGui::Selectable(_label.c_str()))
					{
						// 生成してそのまま選択状態にする
						GameObject::BaseObject* _pNew = _pManager->AddObjectByTypeID(_id);
						if (_pNew)
						{
							a_editContext.pGameObject = _pNew;
							a_editContext.ClearEntitySelection();
						}
						ImGui::CloseCurrentPopup();
					}
				}
			}
			ImGui::EndPopup();
		}

		const auto& _objects = _pManager->GetObjects();

		ImGui::Text("ObjectNum : %d", static_cast<int>(_objects.size()));
		ImGui::Separator();

		// 選択中ポインタがまだ生きているか検証(破棄やシーン切り替えでダングリング化するのを防ぐ)
		bool _selectedStillAlive = false;

		ImGui::BeginChild("GameObjectList");
		{
			for (size_t _i = 0; _i < _objects.size(); ++_i)
			{
				GameObject::BaseObject* _pObj = _objects[_i].get();
				if (!_pObj) continue;

				if (_pObj == a_editContext.pGameObject) _selectedStillAlive = true;

				// ラベル : 表示名 + インデックスで一意化
				std::string _label = _pObj->GetEditorName();
				_label += "##" + std::to_string(_i);

				bool _isSelected = (a_editContext.pGameObject == _pObj);
				if (ImGui::Selectable(_label.c_str(), _isSelected))
				{
					// ゲームオブジェクト選択に切り替え(ECSエンティティの選択は解除)
					a_editContext.pGameObject = _pObj;
					a_editContext.ClearEntitySelection();
					_selectedStillAlive = true;
				}

				// 右クリックで削除
				if (ImGui::BeginPopupContextItem(_label.c_str()))
				{
					if (ImGui::MenuItem("Destroy"))
					{
						_pObj->RequestDestroy();
						if (a_editContext.pGameObject == _pObj)
						{
							a_editContext.pGameObject = nullptr;
						}
					}
					ImGui::EndPopup();
				}
			}
		}
		ImGui::EndChild();

		// 選択中オブジェクトが一覧に無くなっていたら選択解除
		if (a_editContext.pGameObject && !_selectedStillAlive)
		{
			a_editContext.pGameObject = nullptr;
		}
	}
}
