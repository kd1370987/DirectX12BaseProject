#pragma once

#include "Engine/Graphics/RenderingPipeline/RenderingPipeline.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// FishEyePass
	//
	// 入力の絵を魚眼レンズのように歪ませる。
	//
	// 調整値はこのパスのメンバとして持つ。
	// もとはカメラのコンポーネントから GraphicsEngine 経由で毎フレーム流し込んでいたが、
	// パイプラインごとに違う設定にできるよう、パスの持ち物にしてある
	//======================================================================================
	class FishEyePass : public Pass
	{
	public:
		~FishEyePass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

	private:

		// シェーダーへ送る調整値
		FishEyeOptionCB m_cb = { { 0.5f, 0.5f }, 0.0f, 1 };
	};
}
