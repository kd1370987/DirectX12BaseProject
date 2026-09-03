#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// ZPrePass
	//
	// 不透明モデルの深度だけを先に書く。
	// あとの GBuffer が深度テスト EQUAL で描けるようになり、
	// 見えないピクセルのピクセルシェーダーを走らせずに済む
	//======================================================================================
	class ZPrePass : public Pass
	{
	public:
		~ZPrePass() override = default;

		// シェーディングモデル表はこの名前で引く(表示名とは別)
		const char* GetShadingPassName() const override { return "ZPre"; }

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;
	};
}
