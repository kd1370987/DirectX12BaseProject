#pragma once

#include "../../../ComputePass.h"

namespace Engine::Graphics
{

	class GISpatialDenoisePass : public ComputePass
	{
	public:

		GISpatialDenoisePass() = default;
		~GISpatialDenoisePass() override = default;

		void Excute(GraphicsEngine* a_pGE, RenderContext* a_pCtx) override;

	private:

		void CreatePass() override;

	};
}