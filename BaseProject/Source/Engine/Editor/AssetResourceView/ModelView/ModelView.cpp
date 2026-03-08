#include "ModelView.h"

#include "Engine/Graphics/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"
#include "Engine/Resource/Manager/ModelManager/ModelManager.h"

void ModelView::Init()
{
	
}

void ModelView::Draw()
{
	if(ImGui::Begin("ModelStorageView"))
	{
		//UINT _modelSize = GraphicResourceManager::Instance().GetModelResourceStorageSize();

		//for (UINT _i = 0; _i < _modelSize; ++_i)
		//{
		//	auto* _model = GraphicResourceManager::Instance().NGetModel(_i);
		//	if (!_model) continue;
		//	std::string _tagName = "model : " + std::to_string(_i);
		//	if (ImGui::TreeNodeEx(_tagName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
		//	{
		//		// モデル情報描画
		//		DrawModelView(_model);

		//		ImGui::TreePop();
		//	}
		//}

		for (auto& _model : Engine::Resource::ModelManager::Instnace().GetAllModel())
		{
			std::string _tagName = "model";
			if (ImGui::TreeNodeEx(_tagName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
			{
				// モデル情報描画
				DrawModelView(_model.data);

				ImGui::TreePop();
			}
		}
		
	}
	ImGui::End();
}

void ModelView::DrawModelView(Engine::Resource::Model& a_model)
{
	// オリジナルノード
	for (auto& _node : a_model.originalNodes)
	{
		std::string _tagName = _node.name;
		if (ImGui::TreeNodeEx(_tagName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
		{
			// ノード描画
			NodeView(_node);

			ImGui::TreePop();
		}
	}
}

void ModelView::NodeView(Engine::Resource::Node& a_node)
{
	
}

void ModelView::MaterialView(Engine::Resource::Material& a_material)
{
}

void ModelView::MeshView(Engine::Resource::Mesh* a_pMesh)
{
}

void ModelView::AnimationView(Engine::Resource::AnimationData& a_animationData)
{
}

void ModelView::CollisionView(Engine::Resource::Mesh& a_pMesh)
{
}
