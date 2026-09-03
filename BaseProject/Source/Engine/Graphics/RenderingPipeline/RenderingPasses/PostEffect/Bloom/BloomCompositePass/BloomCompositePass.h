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

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

	private:

		BloomOptionCB m_cb = { 1.0f, 0.5f, 1.0f, 1 };
	};
}
