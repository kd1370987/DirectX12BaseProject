#pragma once

#include "../OffScreenPass/OffScreenPass.h"
namespace Engine::Graphics
{
	class DrawPass : public OffScreenPass
	{
	protected:

		void DrawQueue(RenderContext* a_pCtx, RenderQueueType a_type);

	};
}