#include "ShadingModelTableEdit.h"

#include "../../../../../MainEngine.h"
#include "../../../../../Graphics/GraphicEngine.h"
#include "../../../../../Graphics/RenderPassRegistry/RenderPassRegistry.h"

namespace Engine::Editor::Inspector
{
	//-----------------------------------------------------------------------------------------
	// シェーディングモデルテーブルの編集・詳細表示
	//-----------------------------------------------------------------------------------------
	void ShadingModelTableEdit(EditorContext& a_editContext, Resource::ShadingModelTable* a_pTable)
	{
		if (!a_pTable) { return; }

		auto _guid = a_editContext.pAssetProp->guid;

		ImGui::Text("Shading Model: %s", a_pTable->GetName().c_str());
		ImGui::Separator();

		// 保存ボタン
		if (ImGui::Button("Save Asset"))
		{
			auto _filePath = Resource::AssetDatabase::Instance().GetFilePathFromGUID(_guid);
			auto _fileDir = FileUtility::GetDirFromPath(_filePath);
			auto _fileName = FileUtility::GetFileNameWithoutExtension(_filePath);
			Persistence::Archive _ar(Persistence::Archive::Mode::Save, _fileDir, _fileName, "smtble");
			a_pTable->Archive(_ar);
		}

		ImGui::Spacing();

		auto* _pGraphicsEngine = Engine::MainEngine::Instance().RefGraphicsEngine();
		if (!_pGraphicsEngine) { return; }
		auto* _pRenderPassRegistry = _pGraphicsEngine->RefRenderPassRegistry();
		if (!_pRenderPassRegistry) { return; }

		// レンダーパス一覧を取得
		auto& _passNodeVec = _pRenderPassRegistry->RefPassNodes();
		// アセットデータベースからシェーダー一覧を取得
		auto _shaderMetaVec = Resource::AssetDatabase::Instance().GetTypeMetaVec("Shader");

		for (const auto& _passNode : _passNodeVec)
		{
			const auto& _passName = _passNode->name;

			// このパスがテーブルに登録されているか判定
			bool _isActive = a_pTable->HasPass(_passName);

			// =======================================================
			// UI: チェックボックスでパス自体の有効/無効を切り替え
			// =======================================================
			if (ImGui::Checkbox(("##Toggle" + _passName).c_str(), &_isActive))
			{
				if (_isActive)
				{
					a_pTable->EnablePass(_passName);
				}
				else
				{
					a_pTable->DisablePass(_passName);
				}
			}

			ImGui::SameLine();

			if (!ImGui::CollapsingHeader(_passName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) { continue; }

			ImGui::Indent();

			if (!_isActive)
			{
				// 有効になっていない場合は設定させない
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "  (Pass is disabled. Check the box to enable.)");
				ImGui::Unindent();
				continue;
			}

			// 現在このパスに登録されているGUID配列
			const auto& _registeredGUIDs = a_pTable->GetShaderGUIDs(_passName);

			// 登録済みシェーダーの表示と削除
			for (size_t _i = 0; _i < _registeredGUIDs.size(); ++_i)
			{
				// シェーダーアセット名取得
				auto _shaderName = Resource::AssetDatabase::Instance().GetFileNameFromGUID(_registeredGUIDs[_i]);

				ImGui::Text(" %s", _shaderName.c_str());
				ImGui::SameLine(ImGui::GetWindowWidth() - 80.0f);

				// 削除ボタン
				if (ImGui::Button(("Remove##" + _passName + std::to_string(_i)).c_str()))
				{
					// 削除で配列が詰まるため、このフレームの列挙はここで打ち切る
					a_pTable->RemoveShader(_passName, _i);
					break;
				}
			}

			// パスは有効だがPSが登録されていない場合（ZPreやShadowで正しい状態）
			if (_registeredGUIDs.empty())
			{
				ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "  (No PS assigned. Valid for ZPre/Shadow)");
			}

			ImGui::Spacing();

			// 新しいシェーダーを追加するボタンとポップアップ
			if (ImGui::Button(("Add PS Shader##" + _passName).c_str()))
			{
				ImGui::OpenPopup(("SelectShaderPopup##" + _passName).c_str());
			}

			if (ImGui::BeginPopup(("SelectShaderPopup##" + _passName).c_str()))
			{
				ImGui::Text("Select Pixel Shader for %s", _passName.c_str());
				ImGui::Separator();

				for (const auto& _meta : _shaderMetaVec)
				{
					// PS以外は除去
					if (_meta.fileName.find("PS") == std::string::npos) { continue; }

					// 既に登録されているものは選択させない
					bool _isAlreadyAdded = false;
					for (const auto& _shaderGUID : _registeredGUIDs)
					{
						if (_shaderGUID == _meta.guid) { _isAlreadyAdded = true; break; }
					}
					if (_isAlreadyAdded) { continue; }

					if (ImGui::Selectable(_meta.fileName.c_str()))
					{
						a_pTable->AddShader(_passName, _meta.guid);
					}
				}
				ImGui::EndPopup();
			}

			ImGui::Unindent();
		}
	}
}
