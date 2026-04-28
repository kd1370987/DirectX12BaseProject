#pragma once

#include "../RasterizePass.h"
namespace Engine::Graphics
{
	class ZPrePass final : public RasterizePass
	{
	public:

		void Excute(RenderContext* a_pCtx) override;

	private:

		void CreatePass() override;
	};
}