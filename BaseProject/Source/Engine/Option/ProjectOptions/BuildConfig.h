#pragma once

#include "../IOption.h"

namespace Engine::Option::ProjectOptions
{

	/// <summary>
	/// ランタイム時に変更は掛けない
	/// 次の起動時に変更がかかる設定
	/// </summary>
	struct BuildConfig : IOption
	{
		// 環境設定
		EBuildConfiguration buildMode = EBuildConfiguration::Debug;

		// パス設定
		std::string assetRootPath = "Asset/";

		// システムリソース
		UINT maxThreadCount = 4;			// 使用できるスレッド最大数

		const std::string& GetName() override
		{
			static const std::string _name = "BuildConfig";
			return _name;
		}

		EOptionCategory GetCategory() override
		{
			return EOptionCategory::Project;
		}

		void DrawEdit() override
		{
			Editor::DrawEnumCombo("BuildMode",buildMode);
			ImGui::InputText("AssetRootPath",&assetRootPath);
			int _count = (int)maxThreadCount;
			ImGui::DragInt("maxThreadCount",&_count);
			maxThreadCount = (UINT)_count;
		}

		void Archive(Persistence::Archive& a_archive) override
		{	
			a_archive.Field("buildMode",buildMode);
			a_archive.Field("assetRootPath",assetRootPath);
			a_archive.Field("maxThreadCount",maxThreadCount);
		}
	};
}