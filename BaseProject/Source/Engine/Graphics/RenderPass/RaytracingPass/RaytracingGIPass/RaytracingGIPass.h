#pragma once

#include "../RaytracingPass.h"
namespace Engine::Graphics
{
	class RaytracingGIPass final : public RaytracingPass
	{
	public:

		void Excute(RenderContext* a_pCtx) override;

	private:

		void CreatePass() override;
	};
}