#pragma once

#include "Engine/Graphics/RenderingPipeline/RenderingPipeline.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// DeferredLightingPass
	//
	// GBuffer と影・GI を合わせて、最終的な色(AfterLighting)を作る。
	//
	// ライトの配列はグラフのリソースではないので、スロットには乗らない。
	// GraphicsEngine が毎フレーム詰め直したものをここで直接張る
	//======================================================================================
	class DeferredLightingPass : public Pass
	{
	public:
		~DeferredLightingPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

	private:

		// ライティングの調整値
		// ※ HLSL の LightingOptionData と並びを合わせること
		struct LightingOptionCB
		{
			float giIntensity;			// 間接光の強さ
			float directionalIntensity;	// 平行光の強さ
			float dielectricF0;			// 非金属の基準反射率
			float pad;
		};
		LightingOptionCB m_cb = { 1.0f, 1.0f, 0.04f, 0.0f };
	};
}
