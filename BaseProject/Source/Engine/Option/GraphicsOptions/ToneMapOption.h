#pragma once

#include "../IOption.h"

namespace Engine::Option::GraphicsOptions
{
	//======================================================================================
	// トーンマップの種類
	//
	// HDR で組み上げた絵を表示できる 0〜1 へ落とすときの曲線。
	// どれが正解というものではなく絵作りの好みなので、シェーダーを差し替えるのではなく
	// ここから選べるようにしてある。
	//
	// ※ 値は保存されるので、増やすときは必ず末尾に足すこと
	// ※ HLSL 側 TONEMAP_TYPE_*(Asset/Shader/Common/RootParameters/ToneMapOptionData.hlsli)と
	//    数値を合わせること
	//======================================================================================
	enum class EToneMapType : uint32_t
	{
		None,				// 掛けない(0〜1へ切り詰めるだけ。素の明るさを確認したいとき用)
		ACES,				// ACESフィルミック : ハイライトが滑らか。彩度は変わって見える
		Reinhard,			// 白が伸びず、全体的に落ち着いた絵になる
		ReinhardExtended,	// Reinhard に白点指定を足したもの
		Uncharted2,			// フィルミック。強いブルームと相性がいい
	};

	//======================================================================================
	// トーンマップの設定
	//
	// ブルームやライティングと同じく、カメラごとではなくシーン全体の絵作りの設定なので
	// カメラのコンポーネントではなくこちらで持つ。
	//
	// 実際の送信は ToneMapPass が定数バッファへ詰めて行う。
	// ※ HLSL 側(Asset/Shader/Common/RootParameters/ToneMapOptionData.hlsli)と並びを合わせること
	//======================================================================================
	struct ToneMapOption : IOption
	{
		EToneMapType type = EToneMapType::ACES;		// どの曲線で落とすか

		// トーンマップを掛ける前に乗せる露出倍率。
		// 曲線を変えずに全体の明るさだけを動かしたいときに使う
		float exposure = 1.0f;

		// 「この明るさを白として扱う」値。
		// ReinhardExtended と Uncharted2 だけが使う
		float whitePoint = 4.0f;

		const std::string& GetName() override
		{
			static const std::string _name = "ToneMapOption";
			return _name;
		}

		EOptionCategory GetCategory() override
		{
			return EOptionCategory::Graphics;
		}

		void DrawEdit() override
		{
			Editor::EditorHelper::DrawEnumCombo("ToneMapType", type);

			ImGui::DragFloat("Exposure", &exposure, 0.01f, 0.0f, 10.0f);

			// 白点を見ない種類のときは触らせない(効かない値を触れると混乱する)
			const bool _useWhitePoint =
				(type == EToneMapType::ReinhardExtended) || (type == EToneMapType::Uncharted2);

			ImGui::BeginDisabled(!_useWhitePoint);
			ImGui::DragFloat("WhitePoint", &whitePoint, 0.05f, 0.01f, 64.0f);
			ImGui::EndDisabled();
			if (!_useWhitePoint)
			{
				ImGui::TextDisabled("(WhitePoint は ReinhardExtended / Uncharted2 のみ)");
			}
		}

		void Archive(Persistence::Archive& a_archive) override
		{
			a_archive.Field("type", type);
			a_archive.Field("exposure", exposure);
			a_archive.Field("whitePoint", whitePoint);
		}
	};
}
