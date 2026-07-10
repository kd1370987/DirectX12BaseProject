#include "AssetDataBasePanel.h"

namespace Engine::Editor
{
	void AssetDataBasePanel::OnDrawImGui(EditorContext& a_editContext)
	{
		a_editContext.eInspectorType = EInspectorType::Asset;
		AssetDataBaseExplorer(a_editContext);
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