#include "AssetResourceView.h"

#include "ModelView/ModelView.h"
#include "TextureView/TextureView.h"

#include "../../Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../Resource/Loader/StateMachineAsset/StateMachineAssetLoader.h"

namespace Engine::Editor
{
	AssetResourceView::AssetResourceView()
	{}

	AssetResourceView::~AssetResourceView()
	{}

	void AssetResourceView::Init()
	{
		m_upTextureView = std::make_unique<TextureView>();
		m_upTextureView->Init();
	}

	void AssetResourceView::Draw(UINT a_widht, UINT a_height)
	{
		m_upTextureView->Draw(a_widht, a_height);
		// リソースビューの作成
		if (ImGui::Begin("ResourceDataBase"))
		{

			// リソースビューの作成
			ExtensionVec();

			// アセットデータベース描画
			AssetDataBaseDraw();
		}
		ImGui::End();
	}
	void AssetResourceView::ExtensionVec()
	{
		// ステートマシン
		auto& _statePool = Resource::ResourceManager::Instance().RefPool<Resource::StateMachineAsset>();
		if (ImGui::TreeNodeEx("StateMachinAsset", ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
		{
			auto& _pool = _statePool.RefAll();
			for (auto& _stateMachin : _pool)
			{
				if (!_stateMachin.has_value()) continue;
				if (ImGui::TreeNodeEx(_stateMachin->GetName().c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
				{
					_stateMachin->EditImGui();
					ImGui::TreePop();
				}
			}

			ImGui::Separator();

			// ステートマシン追加
			ImGui::InputText("Name",m_nameCach,sizeof(m_nameCach));
			ImGui::InputText("FilePath", m_pathCach, sizeof(m_pathCach));
			if (ImGui::Button("Create"))
			{
				Resource::StateMachineAssetLoader::Create(std::string(m_pathCach), std::string(m_nameCach));
				std::memset(m_nameCach,0,sizeof(m_nameCach));
				std::memset(m_pathCach,0,sizeof(m_pathCach));
			}

			ImGui::TreePop();
		}
	}
	void AssetResourceView::AssetDataBaseDraw()
	{
		// リソース関連一覧
		if (ImGui::TreeNodeEx("Resource"))
		{
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
			ImGui::TreePop();
		}
	}
}