#include "TextureView.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Loader/Texture/TextureLoader.h"
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
			for (auto& [_guid, _handle] : Engine::Resource::TextureLoader::GetAllCache())
			{
				// テクスチャ取得
				auto _pTex = Engine::Resource::ResourceManager::Instance().Ref(_handle);
				if (!_pTex) continue;

				if (ImGui::TreeNodeEx(_pTex->GetName().c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
				{
					ImGui::Text("Handle : IDX = %d GEN = %d", _handle.idx, _handle.gen);
					DrawTextureView(*_pTex, a_widht, a_height);

					ImGui::TreePop();
				}
			}
			for (auto& [_name, _handle] : Engine::Resource::TextureLoader::GetAllNameCache())
			{
				// テクスチャ取得
				auto _pTex = Engine::Resource::ResourceManager::Instance().Ref(_handle);
				if (!_pTex) continue;

				if (ImGui::TreeNodeEx(_pTex->GetName().c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
				{
					ImGui::Text("Handle : IDX = %d GEN = %d", _handle.idx, _handle.gen);
					DrawTextureView(*_pTex, a_widht, a_height);

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
			auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(a_Texture.GetImGuiSRV());
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