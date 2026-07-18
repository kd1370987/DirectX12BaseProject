#include "AssetDataBasePanel.h"

namespace Engine::Editor
{
	AssetDataBasePanel::AssetDataBasePanel()
	{
		m_assetCreateFuncs["AnimatorAsset"] = [](const std::string& path, const std::string& name) {
			Resource::AnimatorAssetIO::Create(path, name);
			};

		m_assetCreateFuncs["ActionStateMachineAsset"] = [](const std::string& path, const std::string& name) {
			Resource::ActionStateMachineAssetIO::Create(path, name);
			};

		m_assetCreateFuncs["ParticlesAsset"] = [](const std::string& path, const std::string& name) {
			Resource::ParticlesAssetIO::Create(path, name);
			};

		m_assetCreateFuncs["ShadingModelTable"] = [](const std::string& path, const std::string& name) {
			Resource::ShadingModelTableIO::Create(path, name);
			};
	}
	void AssetDataBasePanel::OnDrawImGui(EditorContext& a_editContext)
	{
		a_editContext.eInspectorType = EInspectorType::Asset;

		CreateAssetButton(a_editContext);

		AssetDataBaseExplorer(a_editContext);
	}
	void AssetDataBasePanel::CreateAssetButton(EditorContext& a_editContext)
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
	void AssetDataBasePanel::AssetDataBaseExplorer(EditorContext& a_editContext)
	{
		// 再帰的にツリーを描画する関数
		auto _drawNodeFunc = [&]
			(
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

						bool _sel = (a_editContext.pAssetProp && a_editContext.pAssetProp->guid == _asset->guid);
						if (ImGui::Selectable(_asset->fileName.c_str(), _sel))
						{
							a_editContext.pAssetProp = _asset;
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
}