#pragma once

#include "../RaytracingPass.h"
namespace Engine::Graphics
{
	class RaytracingShadowPass final : public RaytracingPass
	{
	public:

		struct GBufferIndex
		{
			int depth;
			int normal;
			DirectX::XMFLOAT2 pad2;
		};

		struct CBLight
		{
			DirectX::XMFLOAT3 dir;
			float pad;
		};

		void Excute(GraphicsEngine* a_pGE, RenderContext* a_pCtx) override;

	private:

		void CreatePass() override;
	};
}