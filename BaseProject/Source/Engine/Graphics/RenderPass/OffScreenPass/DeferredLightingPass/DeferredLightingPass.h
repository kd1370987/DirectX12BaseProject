#pragma once

#include "../OffScreenPass.h"

namespace Engine::Graphics
{
	class DeferredLightingPass final : public OffScreenPass
	{
	public:

		void Excute(RenderContext* a_pCtx) override;

	private:

		void CreatePass() override;
	};
}
