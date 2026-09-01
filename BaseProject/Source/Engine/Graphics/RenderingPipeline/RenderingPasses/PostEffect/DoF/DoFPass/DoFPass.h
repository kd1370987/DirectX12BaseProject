#pragma once

#include "Engine/Graphics/RenderingPipeline/RenderingPipeline.h"

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

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

	private:

		// シェーダーへ送る調整値
		DoFOptionCB m_cb = { 10.0f, 5.0f, 5.0f, 50.0f, 8.0f, 1, 0.0f, 0.0f };
	};
}
