#pragma once

#include "../RasterizePass.h"
namespace Engine::Graphics
{
	class ScreenUIPass final : public RasterizePass
	{
	public:

		void Excute(GraphicsEngine* a_pGE, RenderContext* a_pCtx) override;

	private:

		void CreatePass() override;
	};
}