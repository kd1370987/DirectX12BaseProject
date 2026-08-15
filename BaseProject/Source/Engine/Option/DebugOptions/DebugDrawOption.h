#pragma once

#include "../IOption.h"

namespace Engine::Option::DebugOptions
{
	// デバッグ描画の表示設定。
	//
	// コライダーやレイなどのワイヤー表示(MainEditor::DrawLine/DrawBox/DrawSphere/…)の
	// オンオフをここで持つ。判定は MainEditor 側の入口で行うので、
	// 呼び出し側(システムなど)は今まで通り無条件に Draw～ を呼んでよい。
	// off のときは1本も積まれないため、描画パスもそのまま空振りする。
	struct DebugDrawOption : IOption
	{
		bool drawWire = true;	// false ならデバッグワイヤーを一切描かない

		const std::string& GetName() override
		{
			static const std::string _name = "DebugDrawOption";
			return _name;
		}

		EOptionCategory GetCategory() override
		{
			return EOptionCategory::Debug;
		}

		void DrawEdit() override
		{
			ImGui::Checkbox("Draw Debug Wire", &drawWire);
		}

		void Archive(Persistence::Archive& a_archive) override
		{
			a_archive.Field("drawWire", drawWire);
		}
	};
}
