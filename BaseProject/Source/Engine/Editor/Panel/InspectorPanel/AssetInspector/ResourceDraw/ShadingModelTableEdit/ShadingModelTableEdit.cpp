#include "ShadingModelTableEdit.h"

#include "../../../../../../MainEngine.h"
#include "../../../../../../Graphics/GraphicEngine.h"
#include "../../../../../../Graphics/RenderingPipeline/RenderingPipelineMetaRegistry.h"
#include "../../../../../Helper/EditorHelper.h"

#include "../../AssetLink.h"

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
			auto _fileDir = Engine::File::GetDirFromPath(_filePath);
			auto _fileName = Engine::File::GetFileNameWithoutExtension(_filePath);
			Persistence::Archive _ar(Persistence::Archive::Mode::Save, _fileDir, _fileName, "smtble");
			a_pTable->Archive(_ar);
		}

		ImGui::Spacing();

		auto* _pGraphicsEngine = Engine::MainEngine::Instance().RefGraphicsEngine();
		if (!_pGraphicsEngine) { return; }
		auto* _pPassMetaRegistry = _pGraphicsEngine->RefPassMetaRegistry();
		if (!_pPassMetaRegistry) { return; }

		// アセットデータベースからシェーダー一覧を取得
		auto _shaderMetaVec = Resource::AssetDatabase::Instance().GetTypeMetaVec("Shader");

		//----------------------------------------------------------------------------------
		// 表に並べるパス名を集める
		//
		// 表が持っているのは「パス名 -> ピクセルシェーダー」の対応なので、
		// 並べるのはモデルを受け取るパスだけでよい(GBuffer / ZPre など)。
		// 同じ鍵を複数のパスが使うこともあるので、名前で重複を落とす。
		//
		// 実際に置かれているパスではなく登録済みの型から集めるのは、
		// パイプラインにまだ置いていないパスぶんも先に設定しておけるようにするため
		//----------------------------------------------------------------------------------
		std::vector<std::string> _passNameVec = {};
		for (const auto& [_typeID, _meta] : _pPassMetaRegistry->GetAllMeta())
		{
			if (_meta.shadingPassName.empty()) continue;

			if (std::find(_passNameVec.begin(), _passNameVec.end(), _meta.shadingPassName) != _passNameVec.end())
			{
				continue;
			}
			_passNameVec.push_back(_meta.shadingPassName);
		}

		// 一覧の並びが毎回変わると探しづらいので名前順に固定する
		std::sort(_passNameVec.begin(), _passNameVec.end());

		if (_passNameVec.empty())
		{
			ImGui::TextDisabled("モデルを受け取るパスが登録されていません");
			return;
		}

		for (const auto& _passName : _passNameVec)
		{

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
				// 中身(エントリポイントなど)を見に行けるようにリンクで出す
				DrawAssetLink(&a_editContext, " ", _registeredGUIDs[_i]);

				ImGui::SameLine(ImGui::GetWindowWidth() - 80.0f);

				// 削除ボタン
				if (Engine::Editor::EditorHelper::DeleteButton(("Remove##" + _passName + std::to_string(_i)).c_str()))
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
			if (Engine::Editor::EditorHelper::CreateButton(("Add PS Shader##" + _passName).c_str()))
			{
				ImGui::OpenPopup(("SelectShaderPopup##" + _passName).c_str());
			}

			if (ImGui::BeginPopup(("SelectShaderPopup##" + _passName).c_str()))
			{
				ImGui::Text("Select Pixel Shader for %s", _passName.c_str());
				ImGui::Separator();

				// 数が増えると探せなくなるので名前で絞り込めるようにする
				const std::string& _search = EditorHelper::DrawSearchBox();

				// 同名のシェーダーが別フォルダにあり得るので、置き場所を添える対象を先に拾う
				const auto _duplicatedSet = EditorHelper::CollectDuplicatedNames(
					_shaderMetaVec,
					[](const Resource::AssetProperty& a_prop) { return a_prop.fileName; });

				for (size_t _i = 0; _i < _shaderMetaVec.size(); ++_i)
				{
					const auto& _meta = _shaderMetaVec[_i];

					// PS以外は除去
					if (_meta.fileName.find("PS") == std::string::npos) { continue; }

					// 同名が並ぶときはフォルダ名で絞り込めたほうが早いので、パスも検索対象にする
					if (!EditorHelper::IsMatchSearch(_search, _meta.fileName) &&
						!EditorHelper::IsMatchSearch(_search, _meta.filePath)) { continue; }

					// 既に登録されているものは選択させない
					bool _isAlreadyAdded = false;
					for (const auto& _shaderGUID : _registeredGUIDs)
					{
						if (_shaderGUID == _meta.guid) { _isAlreadyAdded = true; break; }
					}
					if (_isAlreadyAdded) { continue; }

					// 名前が同じでもImGuiのIDがぶつからないようにする
					// (Selectable のIDはラベル文字列から作られるため)
					ImGui::PushID(static_cast<int>(_i));

					const std::string _label = EditorHelper::MakeUniqueLabel(
						_duplicatedSet, _meta.fileName,
						Engine::File::GetDirFromPath(_meta.filePath));

					if (ImGui::Selectable(_label.c_str()))
					{
						a_pTable->AddShader(_passName, _meta.guid);
					}

					ImGui::PopID();
				}
				ImGui::EndPopup();
			}

			ImGui::Unindent();
		}
	}
}
