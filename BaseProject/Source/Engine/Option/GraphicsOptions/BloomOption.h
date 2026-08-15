#pragma once

#include "../IOption.h"

namespace Engine::Option::GraphicsOptions
{
	// 川瀬式ブルームの調整値。
	//
	// 被写界深度と違ってカメラごとに変えるものではなく、シーン全体の絵作りの設定なので
	// カメラのコンポーネントではなくこちらで持つ(ライティング調整値と同じ扱い)。
	//
	// 実際の送信は BloomExtractPass / BloomCompositePass が定数バッファへ詰めて行う。
	// ※ HLSL 側(Asset/Shader/Common/CB/CBBloomOption.hlsli)と並びを合わせること
	struct BloomOption : IOption
	{
		float threshold = 1.0f;		// 高輝度として抽出し始める輝度
		float softKnee  = 0.5f;		// しきい値付近をなめらかにつなぐ幅の割合(0でハードカット)
		float intensity = 0.6f;		// 合成時のブルームの強さ
		bool  enable    = true;		// false ならブルームを掛けない

		const std::string& GetName() override
		{
			static const std::string _name = "BloomOption";
			return _name;
		}

		EOptionCategory GetCategory() override
		{
			return EOptionCategory::Graphics;
		}

		void DrawEdit() override
		{
			ImGui::Checkbox("Enable", &enable);
			ImGui::DragFloat("Threshold", &threshold, 0.01f, 0.0f, 20.0f);
			ImGui::DragFloat("Soft Knee", &softKnee, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 10.0f);
		}

		void Archive(Persistence::Archive& a_archive) override
		{
			a_archive.Field("threshold", threshold);
			a_archive.Field("softKnee", softKnee);
			a_archive.Field("intensity", intensity);
			a_archive.Field("enable", enable);
		}
	};
}
