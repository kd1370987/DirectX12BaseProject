#include "ModelView.h"
//#include "Engine/Resource/Manager/ModelManager/ModelManager.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Loader/Model/ModelLoader.h"
namespace Engine::Editor
{
	void ModelView::Init()
	{

	}

	void ModelView::Draw()
	{
		if (ImGui::Begin("ModelStorageView"))
		{
			for (auto& _model : Engine::Resource::ModelLoader::GetAllCache())
			{
				std::string _tagName = "model";
				if (ImGui::TreeNodeEx(_tagName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
				{
					auto* _pModel = Engine::Resource::ResourceManager::Instance().Ref(_model.second);
					// モデル情報描画
					DrawModelView(*_pModel);

					ImGui::TreePop();
				}
			}

		}
		ImGui::End();
	}

	void ModelView::DrawModelView(const Engine::Resource::Model& a_model)
	{
		// オリジナルノード
		for (auto& _node : a_model.GetOriginalNodeVec())
		{
			std::string _tagName = _node.name;
			if (ImGui::TreeNodeEx(_tagName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
			{
				// ノード描画
				ImGui::TreePop();
			}
		}
	}

	void ModelView::NodeView(Engine::Resource::Node& a_node)
	{

	}

	void ModelView::MaterialView(Engine::Resource::Material& a_material)
	{}

	void ModelView::MeshView(Engine::Resource::Mesh* a_pMesh)
	{}

	void ModelView::AnimationView(Engine::Resource::AnimationData& a_animationData)
	{}

	void ModelView::CollisionView(Engine::Resource::Mesh& a_pMesh)
	{}
}