#pragma once

#include "../IOption.h"

namespace Engine::Option::GraphicsOptions
{
	//======================================================================================
	// スカイの設定
	//
	// スカイ(Sky)シェーディングモデルのマテリアルは GBuffer を通らず、
	// ディファードライティングの後ろの SkyPass で HDR バッファへ直接描かれる。
	// ライティングの計算対象にならないぶん明るさの拠り所が無いので、
	// ここの露出倍率で全体の明るさを合わせる。
	//
	// ブルームやトーンマップと同じくカメラごとではなくシーン全体の絵作りの設定なので、
	// カメラのコンポーネントではなくこちらで持つ。
	//
	// 実際の送信は SkyPass が定数バッファへ詰めて行う。
	// ※ HLSL 側(Asset/Shader/Common/CB/CBSkyOption.hlsli)と並びを合わせること
	//======================================================================================
	struct SkyOption : IOption
	{
		// スカイの色に掛ける露出倍率。
		// 出力先が HDR なので 1.0 を超えて構わない。
		// 超えた分はブルームの抽出しきい値に乗り、最後にトーンマップで落とされる
		float exposure = 1.0f;

		const std::string& GetName() override
		{
			static const std::string _name = "SkyOption";
			return _name;
		}

		EOptionCategory GetCategory() override
		{
			return EOptionCategory::Graphics;
		}

		void DrawEdit() override
		{
			ImGui::DragFloat("Exposure", &exposure, 0.01f, 0.0f, 100.0f);
		}

		void Archive(Persistence::Archive& a_archive) override
		{
			a_archive.Field("exposure", exposure);
		}
	};
}
