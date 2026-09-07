#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// DoFPass
	//
	// CoC の値にしたがって、入力の絵をボカす(被写界深度)。
	//
	// 調整値はこのパスのメンバとして持つ。
	// もとはカメラのコンポーネントから GraphicsEngine 経由で毎フレーム流し込んでいたが、
	// パイプラインごとに違う設定にできるよう、パスの持ち物にしてある
	//======================================================================================
	class DoFPass : public Pass
	{
	public:
		~DoFPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

		// 編集対象の値 : エディターはここだけを触る。
		// 実体は下の m_cb で、シェーダーへはそのまま送られる
		using Params = DoFOptionCB;
		Params& RefParams() { return m_cb; }

	private:

		// シェーダーへ送る調整値
		DoFOptionCB m_cb = { 10.0f, 5.0f, 5.0f, 50.0f, 8.0f, 1, 0.0f, 0.0f };
	};
}
