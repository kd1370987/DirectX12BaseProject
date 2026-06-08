#pragma once

#include "../IOption.h"

namespace Engine::Option::GraphicsOptions
{
	// GIのスペースデノイズの設定
	struct GIOption : IOption
	{
		// テンポラルデノイズ
		float TAphiDepth;			// 深度
		float TAphiNormal;		// ノーマル
		float TAblendRate;		// 過去割合

		// スペースデノイズ
		float	phiDepth = 0;	// 深度の感度（小さいほどエッジを厳密に保護）
		float	phiNormal = 0;	// 法線の感度（大きいほど法線のずれに敏感）
		float	phiColor = 0;	// 輝度の感度（ノイズとディティールの境界制御）

		// カテゴリー
		EOptionCategory GetCategory() override
		{
			return EOptionCategory::Graphics;
		}

		// エディター
		void DrawEdit() override
		{
			// テンポラルデノイズ
			if (ImGui::TreeNodeEx("GITemporalAccumulationOption", ImGuiTreeNodeFlags_SpanFullWidth))
			{
				ImGui::DragFloat("TADepth", &TAphiDepth, 0.01f, 0, 1);
				ImGui::DragFloat("TANormal", &TAphiNormal, 0.1f, 0);
				ImGui::DragFloat("TABlendRate", &TAblendRate, 0.01f, 0, 1);

				ImGui::TreePop();
			}

			// スペースデノイズセッティング
			if (ImGui::TreeNodeEx("GISpatialDenoiseOption", ImGuiTreeNodeFlags_SpanFullWidth))
			{
				ImGui::DragFloat("Depth", &phiDepth, 0.1f, 0);
				ImGui::DragFloat("Normal", &phiNormal, 0.1f, 0);
				ImGui::DragFloat("Color", &phiColor, 0.1f, 0);

				ImGui::TreePop();
			}
		}

		// アーカイブ
		void Archive(Persistence::Archive& a_archive) override
		{
			// TA
			a_archive.Field("TAphiDepth",TAphiDepth);
			a_archive.Field("TAphiNormal", TAphiNormal);
			a_archive.Field("TAblendRate", TAblendRate);

			// スペースデノイズ
			a_archive.Field("phiDepth", phiDepth);
			a_archive.Field("phiNormal", phiNormal);
			a_archive.Field("phiColor", phiColor);
		}
	};
}