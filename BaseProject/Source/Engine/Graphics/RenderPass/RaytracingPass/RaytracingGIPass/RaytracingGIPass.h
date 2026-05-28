#pragma once

#include "../RaytracingPass.h"
namespace Engine::Graphics
{
	class RaytracingGIPass final : public RaytracingPass
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
			int frameCounts;
		};
		void Excute(GraphicsEngine* a_pGE, RenderContext* a_pCtx) override;

	private:

		void CreatePass() override;
		int m_frameCount = 0;
	};
}