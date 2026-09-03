#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// BloomExtractPass
	//
	// 入力から高輝度の部分だけを抜き出す。ブルームの1段目。
	//
	// 調整値はこのパスのメンバ。もとは OptionManager の BloomOption を
	// 抽出パスと合成パスの両方が引いていたが、パイプラインごとに変えられるようにした
	//======================================================================================
	class BloomExtractPass : public Pass
	{
	public:
		~BloomExtractPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

	private:

		BloomOptionCB m_cb = { 1.0f, 0.5f, 1.0f, 1 };
	};
}
