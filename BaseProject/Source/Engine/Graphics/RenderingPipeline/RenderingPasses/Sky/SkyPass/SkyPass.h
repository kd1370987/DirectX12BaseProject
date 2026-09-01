#pragma once

#include "Engine/Graphics/RenderingPipeline/RenderingPipeline.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// SkyPass
	//
	// 何も描かれていないピクセル(深度が最遠)へ、視線方向から引いた空を描く。
	// メッシュは置かず、深度を見て空かどうかを判断する。
	//
	// 空の設定とテクスチャはシーンの SceneAmbientObject の持ち物なので、
	// パスの調整値ではなくエンジンから引く
	//======================================================================================
	class SkyPass : public Pass
	{
	public:
		~SkyPass() override = default;

		void SetupSlots() override;
		void OnLinksResolved() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

		// ルートパラメータ
		static constexpr UINT kRootCameraCB = 0;
		static constexpr UINT kRootSkyCB = 1;
		static constexpr UINT kRootDepthSRV = 2;
		static constexpr UINT kRootSkyTexSRV = 3;
		static constexpr UINT kRootColorUAV = 4;
		static constexpr UINT kRootVelocityUAV = 5;
	};
}
