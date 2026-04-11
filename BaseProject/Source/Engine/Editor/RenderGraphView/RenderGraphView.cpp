#include "RenderGraphView.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
namespace Engine::Editor
{
	void RenderGraphView::Init()
	{}

	void RenderGraphView::Draw()
	{
		if (ImGui::Begin("RGTextureList"))
		{
			std::vector<std::string> _nameList = Engine::Graphics::RenderContext::Instance().GetRGResourceList();

			for (int _i = 0; _i < _nameList.size(); ++_i)
			{
				bool _isSelected = (m_currentSelected == _i);

				if (ImGui::Selectable(_nameList[_i].c_str(), _isSelected))
				{
					m_currentSelected = _i;
				}

				if (_isSelected)
				{
					m_rgTexName = _nameList[_i];
					ImGui::SetItemDefaultFocus();
				}
			}
		}
		ImGui::End();

		// シーンビュー
		if (ImGui::Begin("SceneView"))
		{
			//ImTextureID  _RGTexture = (ImTextureID)(Engine::Graphics::RenderContext::Instance().GetImGuiGPUHandle(m_rgTexName).ptr);
			//ImVec2 _winSize = ImGui::GetContentRegionAvail();

			//float _aspectRatio = 1280.0f / 720.0f;
			//if (_winSize.x / _winSize.y > _aspectRatio)
			//{
			//	_winSize.x = _winSize.y * _aspectRatio;
			//}
			//else
			//{
			//	_winSize.y = _winSize.x / _aspectRatio;
			//}
			//// 最小サイズの設定
			//const float MIN_SIZE = 100.0f;
			//_winSize.x = std::max(_winSize.x, MIN_SIZE);
			//_winSize.y = std::max(_winSize.y, MIN_SIZE);

			//ImGui::Image(_RGTexture, _winSize, ImVec2(0, 0), ImVec2(1, 1));
		}
		ImGui::End();
	}
}