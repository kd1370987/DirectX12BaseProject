#pragma once

#include "../RenderPass.h"

namespace Engine::Graphics
{
	class OffScreenPass : public RenderPass
	{
	protected:
		void Begin(RenderContext* a_pCtx);
		void End(RenderContext* a_pCtx);
	};
}