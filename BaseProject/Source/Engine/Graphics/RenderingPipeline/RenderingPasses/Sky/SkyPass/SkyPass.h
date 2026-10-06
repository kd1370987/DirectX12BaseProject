#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// SkyPass
	//
	// 何も描かれていないピクセル(深度が最遠)へ、視線方向から引いた空を描く。
	// メッシュは置かず、深度を見て空かどうかを判断する。
	//
	// 空の設定とテクスチャはシーン(Engine::Scene::SceneAmbient)の持ち物なので、
	// パスは調整値を持たず SceneView から受け取る
	//======================================================================================
	class SkyPass : public Pass
	{
	public:
		~SkyPass() override = default;

		void SetupSlots() override;
		void OnLinksResolved() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

		// ルートパラメータ
		static constexpr UINT ROOT_CAMERA_CB = 0;
		static constexpr UINT ROOT_SKY_CB = 1;
		static constexpr UINT ROOT_DEPTH_SRV = 2;
		static constexpr UINT ROOT_SKY_TEX_SRV = 3;
		static constexpr UINT ROOT_COLOR_UAV = 4;
		static constexpr UINT ROOT_VELOCITY_UAV = 5;
	};
}
