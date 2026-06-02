#pragma once

#include "../../ComputePass.h"

namespace Engine::Graphics
{

	class TemporalAccumulationPass : public ComputePass
	{
	public:

		TemporalAccumulationPass() = default;
		~TemporalAccumulationPass() override = default;

		void Excute(GraphicsEngine* a_pGE, RenderContext* a_pCtx) override;

	private:

		void CreatePass() override;

	};
}