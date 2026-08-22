#pragma once

#include "../IOption.h"

namespace Engine::Option::GraphicsOptions
{
	// GIのスペースデノイズの設定
	struct GIOption : IOption
	{
		// テンポラルデノイズ
		// ※スペースデノイズ側と名前が似ているが「値の意味」が全く違うので注意。
		//   こちらは棄却判定のしきい値であって、重み関数の係数ではない。
		float TAphiDepth  = 0.05f;	// 位置差の許容量(ビュー深度に対する相対値。0.05 = 5%まで許容)
		float TAphiNormal = 0.9f;	// 法線一致の下限(dotがこれ未満なら履歴を破棄。0〜1の値)
		float TAblendRate = 0.1f;	// 現在フレームの割合(小さいほどノイズは消えるが残像が出る)

		// スペースデノイズ
		float	phiDepth = 0.05f;	// 深度の感度（ビュー深度に対する相対値。小さいほどエッジを厳密に保護）
		float	phiNormal = 32.0f;	// 法線の感度（pow()の指数。大きいほど法線のずれに敏感）
		float	phiColor = 4.0f;	// 輝度の感度（ノイズとディティールの境界制御）

		const std::string& GetName() override
		{
			static const std::string _name = "GIOption";
			return _name;
		}

		// カテゴリー
		EOptionCategory GetCategory() override
		{
			return EOptionCategory::Graphics;
		}

		// エディター
		void DrawEdit() override
		{
			// テンポラルデノイズ
			//if (ImGui::TreeNodeEx("GITemporalAccumulationOption", ImGuiTreeNodeFlags_SpanFullWidth))
			{
				// TANormal は dot積(最大1.0)と比較するしきい値。
				// ここに 32 のような「pow()の指数」用の値を入れると dot < 32 が常に成立して
				// 履歴が毎フレーム全部捨てられ、テンポラルデノイズが完全に無効化される。
				// 事故を防ぐため上限を 1.0 に固定する。
				ImGui::DragFloat("TADepth", &TAphiDepth, 0.005f, 0.0f, 1.0f);
				ImGui::DragFloat("TANormal", &TAphiNormal, 0.005f, 0.0f, 1.0f);
				ImGui::DragFloat("TABlendRate", &TAblendRate, 0.01f, 0.0f, 1.0f);

				//ImGui::TreePop();
			}

			// スペースデノイズセッティング
			ImGui::Separator();
			ImGui::Text("");
			// こちらの Normal は pow() の指数なので 1 を超える値でよい
			ImGui::DragFloat("Depth", &phiDepth, 0.005f, 0.0f, 1.0f);
			ImGui::DragFloat("Normal", &phiNormal, 0.5f, 0.0f, 256.0f);
			ImGui::DragFloat("Color", &phiColor, 0.1f, 0.0f, 100.0f);

		}

		// アーカイブ
		void Archive(Persistence::Archive& a_archive) override
		{
			// TA
			a_archive.Field("TAphiDepth",TAphiDepth);
			a_archive.Field("TAphiNormal", TAphiNormal);
			a_archive.Field("TAblendRate", TAblendRate);

			// 過去に保存された不正値(TAphiNormalに指数用の32が入っている等)を読んでも
			// テンポラルが無効化されないよう、読み込み後に有効域へ丸める
			TAphiNormal = std::clamp(TAphiNormal, 0.0f, 1.0f);
			TAphiDepth  = std::clamp(TAphiDepth, 0.0f, 1.0f);
			TAblendRate = std::clamp(TAblendRate, 0.0f, 1.0f);

			// スペースデノイズ
			a_archive.Field("phiDepth", phiDepth);
			a_archive.Field("phiNormal", phiNormal);
			a_archive.Field("phiColor", phiColor);
		}
	};
}