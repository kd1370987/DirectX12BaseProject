#pragma once

#include "../CopyPass.h"
namespace Engine::Graphics
{
	class GBufferHistoryPass final : public CopyPass
	{
	public:

		void Excute(GraphicsEngine* a_pGE, RenderContext* a_pCtx) override;

	private:

		void CreatePass() override;
	};
}