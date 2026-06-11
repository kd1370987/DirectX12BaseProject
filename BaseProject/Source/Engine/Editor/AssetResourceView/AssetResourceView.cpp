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
}