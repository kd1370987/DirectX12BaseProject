#pragma once

#include "../IOption.h"

namespace Engine::Option::ProjectOptions
{

	/// <summary>
	/// 入力関係の設定
	/// </summary>
	struct InputOption : IOption
	{
		bool isCursorLockedToCenter = false;

		const std::string& GetName() override
		{
			static const std::string _name = "InputOption";
			return _name;
		}

		EOptionCategory GetCategory() override
		{
			return EOptionCategory::Project;
		}

		void DrawEdit() override;
		void Archive(Persistence::Archive& a_archive) override;
	};
}