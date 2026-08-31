#pragma once

#include "../RenderingPipeline.h"

#include "../../../Raytracing/RayPSO/RayPSO.h"
#include "../../../Raytracing/ShaderTable/ShaderTable.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// RaytracingGIPass
	//
	// カメラから見えるピクセルごとにレイを飛ばし、間接光を求める。ハーフ解像度で回る。
	//
	// レイトレはPSOとルートシグネチャを自前で管理するので、
	// グラフの自動バインドは使わない。
	// スロットは依存関係とバリアのためだけに宣言し、バインドは自分で行う
	//======================================================================================
	class RaytracingGIPass : public Pass
	{
	public:
		~RaytracingGIPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

	private:

		// シェーダーへ渡すGBufferのバインドレス添字
		struct GBufferIndex
		{
			int depth;
			int normal;
			int frameCount;		// フレームごとにサンプルをずらすための種
			int pad;
		};

		Raytracing::RayPSO m_rayPSO = {};
		Raytracing::ShaderTable m_shaderTable = {};

		int m_frameCount = 0;
		bool m_isReady = false;
	};
}
