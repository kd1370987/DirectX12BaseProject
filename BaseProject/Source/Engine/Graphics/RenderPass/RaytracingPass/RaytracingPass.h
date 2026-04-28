#pragma once

#include "../BaseRenderPass.h"

#include "../../../Raytracing/RayPSO/RayPSO.h"
#include "../../../Raytracing/ShaderTable/ShaderTable.h"

namespace Engine::Graphics
{
	// 描画用パスクラス
	class RaytracingPass : public BaseRenderPass
	{
	public:

		RaytracingPass() = default;
		virtual ~RaytracingPass() override = default;

	protected:
	
		Raytracing::RayPSO m_rayPSO;
		Raytracing::ShaderTable m_shaderTable;
	};
}