#pragma once
#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// GroundFieldPass
	//
	// GroundDepthPass が描いた地面だけの深度から、地面のワールド座標を復元して書き出す。
	// 地面に広がる波紋などのエフェクトの土台になる。
	//
	// 衝撃(GroundImpulse)と経過時間もシェーダーへ渡している。
	// 衝撃の配列はグラフのリソースではないので、スロットには乗らない。
	// GraphicsEngine が Execute() で今フレームぶんを詰め直したものの番号を直接渡す
	// (衝撃を積むのはアプリ側 : SceneView::AddGroundImpulse)
	//======================================================================================
	class GroundFieldPass : public Pass
	{
	public:
		~GroundFieldPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

	private:

		// ルートパラメータの番号 : シェーダー(GroundFieldCS)の並びと合わせる
		static constexpr int kRootCameraCB = 0;
		static constexpr int kRootGroundFieldCB = 1;
		static constexpr int kRootInputSRV = 2;
		static constexpr int kRootOutputUAV = 3;
		static constexpr int kRootImpulseSRV = 4;

		// パスが回り始めてからの経過時間(秒)。
		// 実行インスタンスごとに持つので、パイプラインを組み直すと 0 から数え直す
		float m_elapsedTime = 0.0f;
	};
}
