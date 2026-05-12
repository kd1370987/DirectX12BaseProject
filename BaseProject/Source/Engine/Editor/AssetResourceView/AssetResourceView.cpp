#include "AssetResourceView.h"

#include "ModelView/ModelView.h"
#include "TextureView/TextureView.h"

#include "../../Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"

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
		ResourceView();
	}
	void AssetResourceView::ResourceView()
	{
		// リソースビューの作成
		if (ImGui::Begin("ResourceDataBase"))
		{

			ExtensionVec();
		}
		ImGui::End();
	}
	void AssetResourceView::ExtensionVec()
	{
		ModelView _modelView;
		// 拡張子ごとに分ける
		for (auto& [_type, _propVec] : Resource::AssetDatabase::Instance().GetTypeMetaMap())
		{
			if (ImGui::TreeNodeEx(_type.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
			{
				// 拡張子ごとのメタデータ配列を作成
				for (auto& _prop : _propVec)
				{
					// ファイル名の表示
					if (ImGui::TreeNodeEx(_prop.fileName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
					{
						if (ImGui::Begin("ModelViewer"))
						{
							_modelView.DrawModel(_prop.guid);
						}
						ImGui::End();
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}
		}
	}
}