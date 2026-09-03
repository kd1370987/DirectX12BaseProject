#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// ParticlePass
	//
	// GPUパーティクルを板ポリで描く。
	// 発生と更新はカメラに依存しないので GraphicsEngine の関数側にあり、
	// このパスは「描く」ところだけを持つ(カメラに向けて板を向けるのでカメラ依存)。
	//
	// 色の重ね方はアセット単位で選べるが、ブレンドはPSOに焼き込まれるので、
	// あらかじめ2つ作っておいて描くときに選ぶ
	//======================================================================================
	class ParticlePass : public Pass
	{
	public:
		~ParticlePass() override = default;

		void SetupSlots() override;
		void OnLinksResolved() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

	private:

		// ブレンドだけが違う2つのPSO。
		// グラフには張らせず(m_psoIndex は無効のまま)、描くときに選ぶ
		Handle<ID3D12PipelineState> m_additivePSO = {};
		Handle<ID3D12PipelineState> m_alphaBlendPSO = {};
	};
}
