#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// FinalOutputPass
	//
	// グラフの出口。どのパイプラインにも必ず1つだけ常駐する。
	//
	// 受け取った絵を、カメラの最終出力(GraphicsEngine が差し込む "CameraOutput")へ
	// そのまま写すだけのパス。シェーダーもPSOも持たず、GPU側はリソースコピー1回で済ませる。
	//
	// これがあることで、パス側は「自分がメインカメラ用かどうか」を知らなくてよくなる。
	// どのカメラも同じ設計図を使い回せる
	//======================================================================================
	class FinalOutputPass : public Pass
	{
	public:
		~FinalOutputPass() override = default;

		// 入口の絵を1本受けて、カメラの最終出力へ写す
		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		// エディター用
		EPassEditResult EditUpdate() override;
		void EditNode() override;

		// シリアライズ : 固有のパラメータを持たない
		void Archive(Engine::Persistence::Archive& a_arch) override;

		// 入口のスロット名 : 常駐ノードなので固定でよい
		static constexpr const char* kInputName = "Color";
		
		// 形が合っていない警告は1回だけ(毎フレーム出すとログが埋まる)
		bool m_isMismatchReported = false;
	};
}
