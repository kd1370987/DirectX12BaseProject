#include "ShadingModelTable.h"

#include "../../Manager/ResourceManager/ResourceManager.h"

#include "../../../MainEngine.h"
#include "../../../Graphics/GraphicEngine.h"
#include "../../../Graphics/RenderPassRegistry/RenderPassRegistry.h"

namespace Engine::Resource
{
	std::span<const ResourceRef<Shader>> Engine::Resource::ShadingModelTable::GetShaderHandles(UINT a_passHash) const
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

			// シェーダーロード
			m_shaderHandleMap.clear();  // ランタイムデータの初期化
			m_activePassHashes.clear(); // パスハッシュリストの初期化

			for (auto& [_pathName, _shaderGUIDVec] : m_shaderGUIDMap)
			{
				UINT _hash = StringUtility::ToHash(_pathName);

				// 有効なパスとしてハッシュを登録
				m_activePassHashes.push_back(_hash);

				// PSが0個でもパス自体は「有効」として扱うため、必ず空配列で初期化する
				m_shaderHandleMap[_hash] = std::vector<ResourceRef<Shader>>();

				for (auto& _shaderGUID : _shaderGUIDVec)
				{
					m_shaderHandleMap[_hash].push_back(ResourceManager::Instance().Load<Shader>(_shaderGUID));
				}
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
			Persistence::Archive _ar(Persistence::Archive::Mode::Save, _dir, _fileName, "smtble");
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

			// このパスがマテリアルに登録されているか判定
			bool _isActive = m_shaderGUIDMap.contains(_passName);

			// =======================================================
			// UI: チェックボックスでパス自体の有効/無効を切り替え
			// =======================================================
			if (ImGui::Checkbox(("##Toggle" + _passName).c_str(), &_isActive))
			{
				UINT _hash = StringUtility::ToHash(_passName);
				if (_isActive)
				{
					// シリアライズ用データの更新
					m_shaderGUIDMap[_passName] = {};
					m_activePasses.push_back(_passName);

					// ランタイム用データの更新
					m_shaderHandleMap[_hash] = {};
					m_activePassHashes.push_back(_hash);
				}
				else
				{
					// シリアライズ用データの更新
					m_shaderGUIDMap.erase(_passName);
					auto _itStr = std::find(m_activePasses.begin(), m_activePasses.end(), _passName);
					if (_itStr != m_activePasses.end()) {
						m_activePasses.erase(_itStr);
					}

					// ランタイム用データからの即時削除
					m_shaderHandleMap.erase(_hash);
					auto _itHash = std::find(m_activePassHashes.begin(), m_activePassHashes.end(), _hash);
					if (_itHash != m_activePassHashes.end()) {
						m_activePassHashes.erase(_itHash);
					}
				}
			}

			ImGui::SameLine();

			if (ImGui::CollapsingHeader(_passName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent();

				if (!_isActive)
				{
					// 有効になっていない場合は設定させない
					ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "  (Pass is disabled. Check the box to enable.)");
				}
				else
				{
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

						// 削除ボタン
						if (ImGui::Button(("Remove##" + _passName + std::to_string(_i)).c_str()))
						{
							// 1. シリアライズ用データ（GUID）から削除
							_registeredGUIDs.erase(_registeredGUIDs.begin() + _i);

							// 2. ランタイム用データ（ハンドル）からも削除を追加
							UINT _hash = StringUtility::ToHash(_passName);
							auto& _handleList = m_shaderHandleMap[_hash];
							if (_i < _handleList.size())
							{
								_handleList.erase(_handleList.begin() + _i);
							}

							_i--;
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
							// .csoとPS以外は除去
							if (_meta.fileName.find("PS") == std::string::npos)
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
									// シリアライズ用データに追加
									_registeredGUIDs.push_back(_meta.guid);

									// ランタイム用データに追加
									UINT _strHash = StringUtility::ToHash(_passName);
									m_shaderHandleMap[_strHash].push_back(ResourceManager::Instance().Load<Shader>(_meta.guid));
								}
							}
						}
						ImGui::EndPopup();
					}
				}

				ImGui::Unindent();
			}
		}
	}
	std::vector<UINT> ShadingModelTable::GetPassHashes() const
	{
		//std::vector<UINT> hashes;
		//hashes.reserve(m_shaderHandleMap.size());
		//for (const auto& [passHash, shaders] : m_shaderHandleMap)
		//{
		//	hashes.push_back(passHash);
		//}
		//return hashes;
		
		return m_activePassHashes; 
		
	}
}