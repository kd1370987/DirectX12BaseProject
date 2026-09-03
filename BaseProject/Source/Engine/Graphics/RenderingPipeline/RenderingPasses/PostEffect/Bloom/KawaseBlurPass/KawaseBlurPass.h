#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// KawaseBlurPass
	//
	// 縮小した4枚を1枚のブルームへまとめる。拡大はサンプリングが兼ねる。
	// 調整値を持たないので、繋ぐだけで動く
	//======================================================================================
	class KawaseBlurPass : public Pass
	{
	public:
		~KawaseBlurPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;
	};
}
