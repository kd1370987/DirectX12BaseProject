#pragma once

#include "../DrawPass.h"
namespace Engine::Graphics
{
	class AnimationZPrePass final : public DrawPass
	{
	public:

		void Excute(RenderContext* a_pCtx) override;

	private:

		void CreatePass() override;
	};
}