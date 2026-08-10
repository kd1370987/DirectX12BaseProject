#pragma once

#include "../IOption.h"

namespace Engine::Option::GraphicsOptions
{
	// 被写界深度(DoF)の調整値。
	// CoCパス(深度→CoC)とDoFパス(CoCでボカす)の両方へ定数バッファで送られる。
	//
	//   ピントが合う範囲 : focusDistance ± focusRange * 0.5
	//   そこから手前へ nearRange 進むと最大ボケ
	//   そこから奥へ   farRange  進むと最大ボケ
	//   最大ボケのときの半径が maxBlurRadius(ピクセル)
	struct DoFOption : IOption
	{
		bool  enable         = false;	// false ならボカさずそのまま通す
		float focusDistance  = 10.0f;	// ピントが合う距離(カメラからの深度)
		float focusRange     =  5.0f;	// ピントが合う幅(この幅の中はボケない)
		float nearRange      =  5.0f;	// 手前側が最大ボケになるまでの距離
		float farRange       = 30.0f;	// 奥側が最大ボケになるまでの距離
		float maxBlurRadius  =  8.0f;	// 最大ボケ半径(ピクセル)

		const std::string& GetName() override
		{
			static const std::string _name = "DoFOption";
			return _name;
		}

		EOptionCategory GetCategory() override
		{
			return EOptionCategory::Graphics;
		}

		void DrawEdit() override
		{
			ImGui::Checkbox("DoF Enable", &enable);
			ImGui::DragFloat("Focus Distance", &focusDistance, 0.1f, 0.0f);
			ImGui::DragFloat("Focus Range", &focusRange, 0.1f, 0.0f);
			ImGui::DragFloat("Near Range", &nearRange, 0.1f, 0.0f);
			ImGui::DragFloat("Far Range", &farRange, 0.1f, 0.0f);
			ImGui::DragFloat("Max Blur Radius", &maxBlurRadius, 0.1f, 0.0f, 32.0f);
		}

		void Archive(Persistence::Archive& a_archive) override
		{
			a_archive.Field("enable", enable);
			a_archive.Field("focusDistance", focusDistance);
			a_archive.Field("focusRange", focusRange);
			a_archive.Field("nearRange", nearRange);
			a_archive.Field("farRange", farRange);
			a_archive.Field("maxBlurRadius", maxBlurRadius);
		}
	};
}
