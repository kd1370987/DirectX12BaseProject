#include "ShadingModelTable.h"

#include "../../Manager/ResourceManager/ResourceManager.h"

#include "../../../MainEngine.h"
#include "../../../Graphics/GraphicEngine.h"
#include "../../../Graphics/RenderPassRegistry/RenderPassRegistry.h"

namespace Engine::Resource
{
	std::span<const Handle<Shader>> Engine::Resource::ShadingModelTable::GetShaderHandles(UINT a_passHash) const
	{
		auto _it = m_shaderHandleMap.find(a_passHash);
		if (_it != m_shaderHandleMap.end())
		{
			return _it->second;
		}
		return {};
	}
	void ShadingModelTable::Archive(Persistence::Archive& a_ar)
	{
		// 基本的なメンバ変数の保存・読み込み
		a_ar.Field("m_typeName", m_typeName);

		// マップの保存・読み込み
		if (a_ar.GetMode() == Persistence::Archive::Mode::Save)
		{
			// ==========================================
			// 保存処理
			// ==========================================
			size_t _mapSize = m_shaderGUIDMap.size();
			if (a_ar.BeginArray("ShaderGUIDMap", _mapSize))
			{
				size_t _idx = 0;
				for (auto& [_passName, _guidVec] : m_shaderGUIDMap)
				{
					if (a_ar.BeginObject(_idx))
					{
						// パス名を保存
						std::string _key = _passName;
						a_ar.Field("PassName", _key);

						// GUID配列を保存
						size_t _guidSize = _guidVec.size();
						if (a_ar.BeginArray("GUIDs", _guidSize))
						{
							for (size_t _i = 0; _i < _guidSize; ++_i)
							{
								if (a_ar.BeginObject(_i))
								{
									a_ar.Field("GUID", _guidVec[_i]);
									a_ar.EndObject();
								}
							}
							a_ar.EndArray();
						}
						a_ar.EndObject();
					}
					_idx++;
				}
				a_ar.EndArray();
			}
		}
		else
		{
			// ==========================================
			// 復元 (Load) 処理
			// ==========================================
			size_t _mapSize = 0;
			if (a_ar.BeginArray("ShaderGUIDMap", _mapSize)) // Load時はここにファイル内の要素数が入る
			{
				m_shaderGUIDMap.clear(); // 既存のデータをクリア

				for (size_t _idx = 0; _idx < _mapSize; ++_idx)
				{
					if (a_ar.BeginObject(_idx))
					{
						// パス名 (Key) を復元
						std::string _passName;
						a_ar.Field("PassName", _passName);

						// GUID配列 (Value) を復元
						std::vector<Engine::GUID> _guidVec;
						size_t _guidSize = 0;
						if (a_ar.BeginArray("GUIDs", _guidSize))
						{
							_guidVec.resize(_guidSize);
							for (size_t _i = 0; _i < _guidSize; ++_i)
							{
								if (a_ar.BeginObject(_i))
								{
									a_ar.Field("GUID", _guidVec[_i]);
									a_ar.EndObject();
								}
							}
							a_ar.EndArray();
						}

						// マップに再登録
						m_shaderGUIDMap.emplace(_passName, _guidVec);
						a_ar.EndObject();
					}
				}
				a_ar.EndArray();
			}
		}
	}
	void ShadingModelTable::Edit(const Engine::GUID& a_guid)
	{
		ImGui::Text("Shading Model: %s", m_typeName.c_str());
		ImGui::Separator();

		// 保存ボタン（パス取得やセーブ処理は仮置き）
		if (ImGui::Button("Save Asset"))
		{
			auto _filePath = AssetDatabase::Instance().GetFilePathFromGUID(a_guid);
			auto _dir = FileUtility::GetDirFromPath(_filePath);
			auto _fileName = FileUtility::GetFileNameWithoutExtension(_filePath);
			Persistence::Archive _ar(Persistence::Archive::Mode::Save,_dir,_fileName,"smtble");
			Archive(_ar);
		}

		ImGui::Spacing();

		auto* _pGE = Engine::MainEngine::Instance().RefGraphicsEngine();
		if (!_pGE) return;
		auto* _pRGPassRegistry = _pGE->RefRenderPassRegistry();
		if (!_pRGPassRegistry) return;

		// レンダーパス一覧を取得
		auto& _passNodeVec = _pRGPassRegistry->RefPassNodes();

		// リソースマネージャーから配列を取得
		auto _shaderMetaVec = AssetDatabase::Instance().GetTypeMetaVec("Shader");

		for (const auto& _passNode : _passNodeVec)
		{
			const auto& _passName = _passNode->name;
			if (ImGui::CollapsingHeader(_passName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent();

				// 現在このパスに登録されているGUID配列の参照を取得
				auto& _registeredGUIDs = m_shaderGUIDMap[_passName];

				// 登録済みシェーダーの表示と削除
				for (size_t _i = 0; _i < _registeredGUIDs.size(); ++_i)
				{
					// シェーダーアセット名取得
					auto _fileName = AssetDatabase::Instance().GetFileNameFromGUID(_registeredGUIDs[_i]);
					std::string _shaderName = _fileName;

					ImGui::Text(" %s", _shaderName.c_str());
					ImGui::SameLine(ImGui::GetWindowWidth() - 80.0f);

					// 削除ボタン（IDの重複を防ぐためパス名とインデックスを付与）
					if (ImGui::Button(("Remove##" + _passName + std::to_string(_i)).c_str()))
					{
						_registeredGUIDs.erase(_registeredGUIDs.begin() + _i);
						_i--;
					}
				}

				// パス内に登録されているシェーダーがなかった場合
				if (_registeredGUIDs.empty())
				{
					ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "  (No shaders assigned - Pass skipped)");
				}

				ImGui::Spacing();

				// 新しいシェーダーを追加するボタンとポップアップ
				if (ImGui::Button(("Add Shader##" + _passName).c_str()))
				{
					ImGui::OpenPopup(("SelectShaderPopup##" + _passName).c_str());
				}

				if (ImGui::BeginPopup(("SelectShaderPopup##" + _passName).c_str()))
				{
					ImGui::Text("Select Pixel Shader for %s", _passName.c_str());
					ImGui::Separator();

					for (const auto& _meta : _shaderMetaVec)
					{
						// .csoとPS以外は除去
						if (_meta.fileName.find(".cso") == std::string::npos ||
							_meta.fileName.find("PS") == std::string::npos)
						{
							continue;
						}

						// 既に登録されているかチェック
						bool _isAlreadyAdded = false;
						for (const auto& _guid : _registeredGUIDs)
						{
							if (_guid == _meta.guid) { _isAlreadyAdded = true; break; }
						}

						// 未登録のものだけを選択可能リストに表示
						if (!_isAlreadyAdded)
						{
							if (ImGui::Selectable(_meta.fileName.c_str()))
							{
								_registeredGUIDs.push_back(_meta.guid);
							}
						}
					}
					ImGui::EndPopup();
				}

				ImGui::Unindent();
			}
		}
	}
	std::vector<UINT> ShadingModelTable::GetPassHashes() const
	{
		std::vector<UINT> hashes;
		hashes.reserve(m_shaderHandleMap.size());
		for (const auto& [passHash, shaders] : m_shaderHandleMap)
		{
			hashes.push_back(passHash);
		}
		return hashes;
	}
}