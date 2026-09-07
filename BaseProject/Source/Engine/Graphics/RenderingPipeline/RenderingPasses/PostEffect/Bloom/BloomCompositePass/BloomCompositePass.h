#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// BloomCompositePass
	//
	// メインカラーへブルームを加算合成する。ブルームの最終段
	//======================================================================================
	class BloomCompositePass : public Pass
	{
	public:
		~BloomCompositePass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

		// 編集対象の値 : エディターはここだけを触る。
		// 実体は下の m_cb で、シェーダーへはそのまま送られる
		using Params = BloomOptionCB;
		Params& RefParams() { return m_cb; }

	private:

		BloomOptionCB m_cb = { 1.0f, 0.5f, 1.0f, 1 };
	};
}
