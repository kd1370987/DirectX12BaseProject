#include "TextureView.h"

#include "Engine/Resource/Manager/TextureManager/TextureManager.h"
#include "../../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

void TextureView::Init()
{
}

void TextureView::Draw()
{
	if (ImGui::Begin("TextureView"))
	{
		for (auto& [_keyName, _handle] : Engine::Resource::TextureManager::Instance().RefAllTex())
		{
			if (ImGui::TreeNodeEx(_keyName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
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
	if (ImGui::Begin("TextureView"))
	{
		auto _gpuHandle = DescriptorHeapManager::Instance().GetSRVGPUHandle(a_Texture.GetSRV());
		ImTextureID _imTex = (ImTextureID)(_gpuHandle.ptr);
		ImVec2 _winSize = ImGui::GetContentRegionAvail();

		float _aspectRatio = 1280.0f / 720.0f;
		if (_winSize.x / _winSize.y > _aspectRatio)
		{
			_winSize.x = _winSize.y * _aspectRatio;
		}
		else
		{
			_winSize.y = _winSize.x / _aspectRatio;
		}
		// 最小サイズの設定
		const float MIN_SIZE = 100.0f;
		_winSize.x = std::max(_winSize.x, MIN_SIZE);
		_winSize.y = std::max(_winSize.y, MIN_SIZE);

		ImGui::Image(_imTex, _winSize, ImVec2(0, 0), ImVec2(1, 1));
	}
	ImGui::End();
}
