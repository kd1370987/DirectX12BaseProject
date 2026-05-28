#pragma once

#include "../RaytracingPass.h"
namespace Engine::Graphics
{
	class FullRaytracingPass final : public RaytracingPass
	{
	public:

		void Excute(GraphicsEngine* a_pGE, RenderContext* a_pCtx) override;

	private:

		void CreatePass() override;
	};
}