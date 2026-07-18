#pragma once

#include "../IOption.h"

namespace Engine::Option::GraphicsOptions
{
	// GIのスペースデノイズの設定
	struct RenderingOption : IOption
	{
	
		bool isZPre = true;

		Engine::GUID defaultShadingModelTable = {};

		const std::string& GetName() override
		{
			return "RenderingOption";
		}

		// カテゴリー
		EOptionCategory GetCategory() override
		{
			return EOptionCategory::Graphics;
		}

		// エディター
		void DrawEdit() override
		{

			if (ImGui::TreeNodeEx("RenderingOption", ImGuiTreeNodeFlags_SpanFullWidth))
			{
				ImGui::Checkbox("isZPre", &isZPre);
				ImGui::TreePop();
			}
		}

		// アーカイブ
		void Archive(Persistence::Archive& a_archive) override
		{
			a_archive.Field("isZPre", isZPre);
			a_archive.Field("defaultShadingModelTable",defaultShadingModelTable);
		}
	};
}