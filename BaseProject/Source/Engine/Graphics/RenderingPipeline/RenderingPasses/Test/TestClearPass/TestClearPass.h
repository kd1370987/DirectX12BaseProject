#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// TestClearPass
	//
	// 配線が通っているかを目で確かめるためのパス。
	// 出力テクスチャを指定色で塗るだけで、シェーダーもPSOも持たない。
	//
	// これを FinalOutputPass へ繋いでカメラに設定すると、
	// 「グラフのコンパイル -> リソース割り当て -> バリア -> パス実行 -> 最終出力」
	// までが一通り動いているかが画面の色で分かる
	//======================================================================================
	class TestClearPass : public Pass
	{
	public:
		~TestClearPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

		// 出力スロット名
		static constexpr const char* kOutputName = "Color";

	private:

		// 塗る色。グラフのクリア指定ではなく自分で塗るので、
		// 色を変えてもリソースを作り直さなくてよい
		Math::Color m_clearColor = { 0.1f, 0.3f, 0.6f, 1.0f };
	};
}
