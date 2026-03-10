#include "TextureView.h"

#include "Engine/Resource/Manager/TextureManager/TextureManager.h"

void TextureView::Init()
{
}

void TextureView::Draw()
{
	if (ImGui::Begin("TextureView"))
	{
		for (auto& [_keyName, _handle] : Engine::Resource::TextureManager::Instance().RefAllTex())
		//for(auto& _tex : Engine::Resource::TextureManager::Instance().GetAllTex())
		{
			if (ImGui::TreeNodeEx(_keyName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
			//if (ImGui::TreeNodeEx(_tex.data.GetName().c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
			{
				ImGui::Text("Handle : IDX = %d GEN = %d",_handle.idx,_handle.gen);

				auto& _tex = Engine::Resource::TextureManager::Instance().RefTexture(_handle);
				DrawTextureView(_tex);

				ImGui::TreePop();
			}
		}
	}
	ImGui::End();
}

void TextureView::DrawTextureView(Engine::Resource::Texture& a_Texture)
{

}
