#pragma once

#include "../IOption.h"

#include "../../Editor/ImGui/ImGuiHelper/ImGuiHelper.h"

namespace Engine::Option::GraphicsOptions
{
	// GIのスペースデノイズの設定
	struct WindowOption : IOption
	{
		// ウィンドウサイズ
		int windowWidth = 0;
		int windowHeight = 0;

		// ウィンドウモード
		EWindowMode windowMode = EWindowMode::Windowed;
		
		// 垂直同期
		bool isVsync = false;

		// 最大フレームレート
		int targetFrameRate = 0;

		const std::string& GetName() override
		{
			return "WindowOption";
		}


		// カテゴリー
		EOptionCategory GetCategory() override
		{
			return EOptionCategory::Graphics;
		}

		// エディター
		void DrawEdit() override
		{
			if (ImGui::TreeNodeEx("WindowOption", ImGuiTreeNodeFlags_SpanFullWidth))
			{
				// ウィンドウサイズ
				ImGui::Text("WindowSize");
				ImGui::Text("Width : %f",windowWidth);
				ImGui::Text("Height : %f",windowHeight);
				ImGui::DragInt("Width", &windowWidth, 1, 0, 1980);
				ImGui::DragInt("Height", &windowHeight, 1, 0, 1080);

				ImGui::Separator();

				// ウィンドウモード
				Editor::DrawEnumCombo("WindowMode", windowMode);
				ImGui::Checkbox("Vsync",&isVsync);
				ImGui::DragInt("TargetFrameRate", &targetFrameRate, 1, 0, 1000);

				ImGui::Separator();

				ImGui::TreePop();
			}

		}

		// アーカイブ
		void Archive(Persistence::Archive& a_archive) override
		{
			// ウィンドウサイズ
			a_archive.Field("windowWidth", windowWidth);
			a_archive.Field("windowHeight", windowHeight);

			// モード
			UINT _winMode = static_cast<UINT>(windowMode);
			a_archive.Field("windowMode",_winMode);
			windowMode = static_cast<EWindowMode>(_winMode);

			a_archive.Field("isVsync",isVsync);
			a_archive.Field("targetFrameRate",targetFrameRate);
		}
	};
}