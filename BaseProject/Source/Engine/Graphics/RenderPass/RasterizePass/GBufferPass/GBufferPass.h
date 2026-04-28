#pragma once

#include "../RasterizePass.h"
namespace Engine::Graphics
{
	class GBufferPass final : public RasterizePass
	{
	public:

		void Excute(RenderContext* a_pCtx) override;

	private:

		void CreatePass() override;
	};
}