#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// RadialBlurPass
	//
	// 入力の絵を、指定した中心から放射状に引きずってボカす。
	//
	// 調整値はこのパスのメンバとして持つ。
	// もとはカメラのコンポーネントから GraphicsEngine 経由で毎フレーム流し込んでいたが、
	// パイプラインごとに違う設定にできるよう、パスの持ち物にしてある
	//======================================================================================
	class RadialBlurPass : public Pass
	{
	public:
		~RadialBlurPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

		// 編集対象の値 : エディターはここだけを触る。
		// 実体は下の m_cb で、シェーダーへはそのまま送られる
		using Params = RadialBlurOptionCB;
		Params& RefParams() { return m_cb; }

	private:

		// シェーダーへ送る調整値
		RadialBlurOptionCB m_cb = { { 0.5f, 0.5f }, 0.02f, 8, 0.1f, 1.0f, 1, 0.0f };
	};
}
