#include "InspectorPanel.h"

#include "AssetInspector/AssetInspector.h"

#include "EntityInspector/EntityInspector.h"

#include "Engine/GameObject/BaseObject/BaseObject.h"
#include "Engine/GameObject/GameObjectManager/GameObjectManager.h"
#include "Engine/Scene/SceneManager/SceneManager.h"

// AssetInspector を前方宣言で持つので、生成と破棄はここ(完全型が見える場所)に置く
Editor::InspectorPanel::InspectorPanel()
	: m_upAssetInspector(std::make_unique<Inspector::AssetInspector>())
{}

Editor::InspectorPanel::~InspectorPanel() = default;

void Editor::InspectorPanel::OnDrawImGui(EditorContext& a_editContext)
{
	switch (a_editContext.eInspectorType)
	{
	case EInspectorType::None :
		Engine::EditorField::HelpText("No selected");
		break;
	case EInspectorType::Entity:
		Inspector::EntityInspector(a_editContext);
		break;
	case EInspectorType::Asset:
		if (m_upAssetInspector) m_upAssetInspector->Draw(a_editContext);
		break;
	case EInspectorType::Game:
		// GameObjectManager管理下のオブジェクトのエディターを描画
		if (a_editContext.pGameObject)
		{
			// オブジェクト側がシングルトンを触らずに済むよう、
			// マネージャーが配っているものと同じ実行コンテキストを渡す
			auto* _pManager = a_editContext.pServices->pSceneManager->RefGameObjectManager();
			if (!_pManager) break;

			// 今のシーンのオブジェクトかどうかを、中身を触る前に確かめる。
			// このパネルは GameObjectHierarchyPanel より先に描かれるので、
			// あちらの選択検証はまだ回っていない。
			// シーン切り替え直後は解放済みのポインタが残っていることがある
			if (!_pManager->IsManaged(a_editContext.pGameObject))
			{
				a_editContext.pGameObject = nullptr;
				Engine::EditorField::HelpText("No selected object");
				break;
			}

			Engine::EditorField::Text("%s", a_editContext.pGameObject->GetEditorName());

			//--------------------------------------------------------------
			// 自身のGUID
			//
			// 進行役(HomeSequence / MissionSelect など)は相手を GUID で指すので、
			// 目当てのオブジェクトを開いたときにここから拾えるようにしておく
			//--------------------------------------------------------------
			{
				const std::string _guid = a_editContext.pGameObject->GetGUID().String();

				Engine::EditorField::HelpText("GUID : %s", _guid.c_str());
				Engine::EditorField::SameLine();
				if (ImGui::SmallButton("Copy")) ImGui::SetClipboardText(_guid.c_str());

				// ヒエラルキー上の親(並びのまとまりだけ。座標も表示も伝わらない)
				const Core::GUID& _parentGUID = a_editContext.pGameObject->GetParentGUID();
				if (_parentGUID.IsValid())
				{
					const auto* _pParent = _pManager->FindByGUID(_parentGUID);

					Engine::EditorField::HelpText("Parent : %s", _pParent ? _pParent->GetEditorName() : "(missing)");
					Engine::EditorField::SameLine();
					if (ImGui::SmallButton("Unparent"))
					{
						a_editContext.pGameObject->SetParentGUID({});
					}
				}
			}

			Engine::EditorField::Line();
			a_editContext.pGameObject->DrawInspector(_pManager->RefObjectContext());
		}
		else
		{
			Engine::EditorField::HelpText("No selected object");
		}
		break;
	default:
		break;
	}
}
