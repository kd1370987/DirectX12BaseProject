#include "AssetResourceView.h"

#include "ModelView/ModelView.h"
#include "TextureView/TextureView.h"

#include "../../Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Editor
{
	AssetResourceView::AssetResourceView()
	{}

	AssetResourceView::~AssetResourceView()
	{}

	void AssetResourceView::Init()
	{
		m_assetCreateFuncs["StateMachinAsset"] = [](const std::string& path, const std::string& name) {
			Resource::StateMachineAssetLoader::Create(path, name);
		};

		m_assetCreateFuncs["ParticlesAsset"] = [](const std::string& path, const std::string& name) {
			Resource::ParticlesAssetLoader::Create(path, name);
		};

		m_assetCreateFuncs["ShadingModelTable"] = [](const std::string& path, const std::string& name) {
			Resource::ShadingModelTableLoader::Create(path, name);
		};
	}

	void AssetResourceView::Draw(UINT a_widht, UINT a_height)
	{
		// リソースビューの作成
		if (ImGui::Begin("ResourceDataBase"))
		{
			// 作成ボタン
			CreateAssetButton();

			ImGui::Separator();

			// アセットデータベース描画
			AssetDataBaseDraw();
		}
		ImGui::End();

		// 各個別に詳細描画
		ResourceView();
	}
	void AssetResourceView::AssetDataBaseDraw()
	{
		// リソース関連一覧
			// 再帰的にツリーを描画する関数
		auto _drawNodeFunc = [&](
			const std::string& a_name,
			const Resource::AssetNode& a_node,
			const std::string& a_tabName,
			auto& a_self
			)
			{
				// 子ノードの中にこのタブのフィルターに一致するアセットが一つでもあるか確認
				auto _hasMatchingAsset = [&](
					const Resource::AssetNode& a_node,
					const std::string& a_filter,
					auto& a_checkSelf
					) -> bool
					{
						if (a_filter == "All") return true;
						for (auto* a : a_node.assets) { if (a->type == a_filter) return true; }
						for (auto& c : a_node.children) { if (a_checkSelf(c.second, a_filter, a_checkSelf)) return true; }
						return false;
					};

				// アセットが入っていなければリターン
				if (!_hasMatchingAsset(a_node, a_tabName, _hasMatchingAsset)) return;

				bool _nodeOpen = true;

				// 名前が空の場合はルートノードなので中身を描画する
				auto _defaultFlag = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed;
				if (!a_name.empty())
				{
					_nodeOpen = ImGui::TreeNodeEx(a_name.c_str(), _defaultFlag | ImGuiTreeNodeFlags_DefaultOpen);
				}

				// 上階層が開いていたら子ノードを描画
				if (_nodeOpen)
				{
					// フォルダ（子ノードを描画）
					for (auto& _child : a_node.children)
					{
						a_self(_child.first, _child.second, a_tabName, a_self);
					}

					// ファイル（アセットを描画）
					for (auto* _asset : a_node.assets)
					{
						// Allタブ以外の場合、Typeが一致しないファイルを除外
						if (a_tabName != "All" && _asset->type != a_tabName) { continue; }

						bool _sel = (m_pAssetPropCach && m_pAssetPropCach->guid == _asset->guid);
						if (ImGui::Selectable(_asset->fileName.c_str(), _sel))
						{
							m_pAssetPropCach = _asset;
						}
					}
					if (!a_name.empty())
					{
						ImGui::TreePop();
					}
				}
			};

		// タブバーを作成
		if (ImGui::BeginTabBar("AssetTabs"))
		{
			// アセットの構造階層を取得
			const auto& _rootNode = Resource::AssetDatabase::Instance().GetAssetRootNode();
			const auto& _types = Resource::AssetDatabase::Instance().GetAssetTypeExtensionsMap();
			for (auto& [_type, _typeExt] : _types)
			{
				if (ImGui::BeginTabItem(_type.c_str()))
				{
					// キャッシュされたツリーを描画
					_drawNodeFunc("", _rootNode, _type, _drawNodeFunc);
					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();
		}
	}
	void AssetResourceView::ResourceView()
	{
		if (ImGui::Begin("ResourceView"))
		{
			if (!m_pAssetPropCach)
			{
				ImGui::Text("Not select resource");
			}
			else
			{
				ImGui::Text("GUID");
				ImGui::Spacing();
				ImGui::Text("%s", m_pAssetPropCach->guid.String().c_str());
				ImGui::Separator();

				ImGui::Text("FilePath");
				ImGui::Spacing();
				for (auto& _ext : m_pAssetPropCach->extensionsVec)
				{
					auto _filePath = m_pAssetPropCach->filePath + m_pAssetPropCach->fileName + _ext;
					ImGui::Text("%s", _filePath.c_str());
				}
				ImGui::Separator();
				auto& _guid = m_pAssetPropCach->guid;
				auto& _type = m_pAssetPropCach->type;
				if (_type == "Model")
				{
					DrawModel();
				}
				else if (_type == "Mesh")
				{

				}
				else if (_type == "Material")
				{
					if (Resource::ResourceManager::Instance().Has<Resource::Material>(_guid))
					{
						auto _handle = Resource::ResourceManager::Instance().Load<Resource::Material>(_guid);
						auto* _pData = Resource::ResourceManager::Instance().Ref(_handle);
						if (!_pData)
						{
							ImGui::Text("Not faund particle");
							return;
						}
						_pData->Edit(_guid);
					}
					else
					{
						ImGui::Text("No loaded file");
						if (ImGui::Button("Load"))
						{
							Resource::ResourceManager::Instance().Load<Resource::Material>(_guid);
						}
					}
				}
				else if (_type == "Animation")
				{

				}
				else if (_type == "StateMachinAsset")
				{
					DrawStateMachin();
				}
				else if (_type == "Texture")
				{
					TextureInspecter(m_pAssetPropCach->guid);
				}
				else if (_type == "Shader")
				{

				}
				else if (_type == "ParticlesAsset")
				{
					DrawParticlesAsset();
				}
				else if (_type == "ShadingModelTable")
				{
					auto& _guid = m_pAssetPropCach->guid;
					auto& _type = m_pAssetPropCach->type;
					if (Resource::ResourceManager::Instance().Has<Resource::ShadingModelTable>(_guid))
					{
						auto _handle = Resource::ResourceManager::Instance().Load<Resource::ShadingModelTable>(_guid);
						auto* _pData = Resource::ResourceManager::Instance().Ref(_handle);
						if (!_pData)
						{
							ImGui::Text("Not faund particle");
							return;
						}
						_pData->Edit(_guid);
					}
					else
					{
						ImGui::Text("No loaded file");
						if (ImGui::Button("Load"))
						{
							Resource::ResourceManager::Instance().Load<Resource::ShadingModelTable>(_guid);
						}
					}
				}
			}
		}
		ImGui::End();
	}
	void AssetResourceView::DrawModel()
	{
		auto& _guid = m_pAssetPropCach->guid;
		auto& _type = m_pAssetPropCach->type;
		if(Resource::ResourceManager::Instance().Has<Resource::Model>(_guid))
		{
			auto _handle = Resource::ResourceManager::Instance().GetCache<Resource::Model>(_guid);
			auto* _pModel = Resource::ResourceManager::Instance().Ref(_handle);
			if (!_pModel)
			{
				ImGui::Text("Not faund model");
				return;
			}
			else
			{
				if (ImGui::Button("Convert"))
				{
					auto _path = Resource::AssetDatabase::Instance().GetFilePathFromGUID(_guid);
					_pModel->Save(_path);
				}
			}
		}
		else
		{
			ImGui::Text("No loaded file");
			if (ImGui::Button("Load"))
			{
				Resource::ResourceManager::Instance().Load<Resource::Model>(_guid);
			}
		}
	}
	void AssetResourceView::DrawStateMachin()
	{
		auto& _guid = m_pAssetPropCach->guid;
		auto& _type = m_pAssetPropCach->type;
		if (Resource::ResourceManager::Instance().Has<Resource::StateMachineAsset>(_guid))
		{
			auto _handle = Resource::ResourceManager::Instance().Load<Resource::StateMachineAsset>(_guid);
			auto* _pMachin = Resource::ResourceManager::Instance().Ref(_handle);
			if (!_pMachin)
			{
				ImGui::Text("Not faund state machin");
				return;
			}
			_pMachin->EditImGui(_handle);
		}
		else
		{
			ImGui::Text("No loaded file");
			if (ImGui::Button("Load"))
			{
				Resource::ResourceManager::Instance().Load<Resource::StateMachineAsset>(_guid);
			}
		}
	}
	void AssetResourceView::DrawParticlesAsset()
	{
		auto& _guid = m_pAssetPropCach->guid;
		auto& _type = m_pAssetPropCach->type;
		if (Resource::ResourceManager::Instance().Has<Resource::ParticlesAsset>(_guid))
		{
			auto _handle = Resource::ResourceManager::Instance().Load<Resource::ParticlesAsset>(_guid);
			auto* _pData = Resource::ResourceManager::Instance().Ref(_handle);
			if (!_pData)
			{
				ImGui::Text("Not faund particle");
				return;
			}
			_pData->EditImGui();
		}
		else
		{
			ImGui::Text("No loaded file");
			if (ImGui::Button("Load"))
			{
				Resource::ResourceManager::Instance().Load<Resource::ParticlesAsset>(_guid);
			}
		}
	}
	void AssetResourceView::CreateAssetButton()
	{
		if (ImGui::Button("Create New Asset..."))
		{
			ImGui::OpenPopup("CreateResourcePopup");
		}

		// ポップアップの中身
		if (ImGui::BeginPopup("CreateResourcePopup"))
		{
			ImGui::Text("Select Asset Type:");
			ImGui::Separator();

			// データベースから現在登録されている全てのアセットタイプを取得
			auto _typeMap = Resource::AssetDatabase::Instance().GetAssetTypeExtensionsMap();

			// ループで動的にUIを生成する！
			for (const auto& [_typeName, _extensions] : _typeMap)
			{
				// ファクトリに登録されていないタイプ（Createできないもの）はスキップする安全策
				if (!m_assetCreateFuncs.contains(_typeName)) continue;

				// ツリーノードの生成
				if (ImGui::TreeNodeEx(_typeName.c_str()))
				{
					ImGui::InputText("Name", m_nameCach, sizeof(m_nameCach));
					ImGui::InputText("FilePath", m_pathCach, sizeof(m_pathCach));

					if (ImGui::Button("Create"))
					{
						// 辞書から該当する関数を引っ張ってきて実行！
						m_assetCreateFuncs[_typeName](std::string(m_pathCach), std::string(m_nameCach));

						// キャッシュのクリア
						std::memset(m_nameCach, 0, sizeof(m_nameCach));
						std::memset(m_pathCach, 0, sizeof(m_pathCach));

						// 作成したらポップアップを閉じる
						ImGui::CloseCurrentPopup();
					}
					ImGui::TreePop();
				}
			}
			ImGui::EndPopup();
		}
	}
}