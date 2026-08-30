#pragma once

#include "../RenderingPipeline.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// DebugLinePass
	//
	// 当たり判定やレイなどのデバッグ線を描く。深度は読むだけで書かない
	//======================================================================================
	class DebugLinePass : public Pass
	{
	public:
		~DebugLinePass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;
	};
}
