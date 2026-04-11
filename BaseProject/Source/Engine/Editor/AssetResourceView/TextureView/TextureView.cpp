#include "TextureView.h"

#include "Engine/Resource/Manager/TextureManager/TextureManager.h"
#include "../../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
namespace Engine::Editor
{
	void TextureView::Init()
	{
		m_minSize = 100.f;
	}

	void TextureView::Draw(UINT a_widht, UINT a_height)
	{
		if (ImGui::Begin("TextureView"))
		{
			for (auto& [_keyName, _handle] : Engine::Resource::TextureManager::Instance().RefAllTex())
			{
				if (ImGui::TreeNodeEx(_keyName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
				{
					ImGui::Text("Handle : IDX = %d GEN = %d", _handle.idx, _handle.gen);

					auto& _tex = Engine::Resource::TextureManager::Instance().RefTexture(_handle);
					DrawTextureView(_tex, a_widht, a_height);

					ImGui::TreePop();
				}
			}
		}
		ImGui::End();
	}

	void TextureView::DrawTextureView(Engine::Resource::Texture& a_Texture, UINT a_widht, UINT a_height)
	{
		if (ImGui::Begin("TextureView"))
		{
			auto _gpuHandle = DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(a_Texture.GetImGuiSRV());
			ImTextureID _imTex = (ImTextureID)(_gpuHandle.ptr);
			ImVec2 _winSize = ImGui::GetContentRegionAvail();

			float _aspectRatio = (float)a_widht / (float)a_height;
			if (_winSize.x / _winSize.y > _aspectRatio)
			{
				_winSize.x = _winSize.y * _aspectRatio;
			}
			else
			{
				_winSize.y = _winSize.x / _aspectRatio;
			}
			// 最小サイズの設定
			_winSize.x = std::max(_winSize.x, m_minSize);
			_winSize.y = std::max(_winSize.y, m_minSize);

			ImGui::Image(_imTex, _winSize, ImVec2(0, 0), ImVec2(1, 1));
		}
		ImGui::End();
	}
}