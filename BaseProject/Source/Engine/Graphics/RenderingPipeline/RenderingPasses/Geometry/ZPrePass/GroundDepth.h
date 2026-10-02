#pragma once
#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	class GroundDepthPass : public Pass
	{
	public:
		~GroundDepthPass() override = default;

		// シェーディングモデル表はこの名前で引く(表示名とは別)
		const char* GetShadingPassName() const override { return "GroundDepth"; }

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;
	};
}