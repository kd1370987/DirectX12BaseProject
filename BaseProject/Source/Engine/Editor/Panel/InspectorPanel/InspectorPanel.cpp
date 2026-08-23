#include "InspectorPanel.h"

#include "AssetInspector/AssetInspector.h"

#include "EntityInspector/EntityInspector.h"

#include "../../../GameObject/BaseObject/BaseObject.h"
#include "../../../GameObject/GameObjectManager/GameObjectManager.h"
#include "../../../Scene/SceneManager/SceneManager.h"

void Engine::Editor::InspectorPanel::OnDrawImGui(EditorContext& a_editContext)
{
	switch (a_editContext.eInspectorType)
	{
	case EInspectorType::None :
		ImGui::Text("No selected");
		break;
	case EInspectorType::Entity:
		Inspector::EntityInspector(a_editContext);
		break;
	case EInspectorType::Asset:
		Inspector::AssetInspector(a_editContext);
		break;
	case EInspectorType::Game:
		// GameObjectManager管理下のオブジェクトのエディターを描画
		if (a_editContext.pGameObject)
		{
			// オブジェクト側がシングルトンを触らずに済むよう、
			// マネージャーが配っているものと同じ実行コンテキストを渡す
			auto* _pManager = Engine::Scene::SceneManager::Instance().RefGameObjectManager();
			if (!_pManager) break;

			// 今のシーンのオブジェクトかどうかを、中身を触る前に確かめる。
			// このパネルは GameObjectHierarchyPanel より先に描かれるので、
			// あちらの選択検証はまだ回っていない。
			// シーン切り替え直後は解放済みのポインタが残っていることがある
			if (!_pManager->IsManaged(a_editContext.pGameObject))
			{
				a_editContext.pGameObject = nullptr;
				ImGui::Text("No selected object");
				break;
			}

			ImGui::Text("%s", a_editContext.pGameObject->GetEditorName());

			//--------------------------------------------------------------
			// 自身のGUID
			//
			// 進行役(HomeSequence / MissionSelect など)は相手を GUID で指すので、
			// 目当てのオブジェクトを開いたときにここから拾えるようにしておく
			//--------------------------------------------------------------
			{
				const std::string _guid = a_editContext.pGameObject->GetGUID().String();

				ImGui::TextDisabled("GUID : %s", _guid.c_str());
				ImGui::SameLine();
				if (ImGui::SmallButton("Copy")) ImGui::SetClipboardText(_guid.c_str());

				// ヒエラルキー上の親(並びのまとまりだけ。座標も表示も伝わらない)
				const Engine::GUID& _parentGUID = a_editContext.pGameObject->GetParentGUID();
				if (_parentGUID.IsValid())
				{
					const auto* _pParent = _pManager->FindByGUID(_parentGUID);

					ImGui::TextDisabled("Parent : %s",
						_pParent ? _pParent->GetEditorName() : "(missing)");
					ImGui::SameLine();
					if (ImGui::SmallButton("Unparent"))
					{
						a_editContext.pGameObject->SetParentGUID({});
					}
				}
			}

			ImGui::Separator();
			a_editContext.pGameObject->DrawInspector(_pManager->RefObjectContext());
		}
		else
		{
			ImGui::Text("No selected object");
		}
		break;
	default:
		break;
	}
}
