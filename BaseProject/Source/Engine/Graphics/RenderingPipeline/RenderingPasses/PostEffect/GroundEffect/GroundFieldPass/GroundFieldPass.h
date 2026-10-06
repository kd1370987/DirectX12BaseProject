#pragma once
#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// GroundFieldPass
	//
	// 衝撃(GroundImpulse)が地面のチリをどう動かしたかを、真上から見たテクスチャへ書く。
	// カメラを中心にした GROUND_FIELD_WORLD_SIZE (m) 四方を xz で並べたもの(解像度固定)。
	//   r = 払われずに残ったチリの量 / g = 波頭に寄せられたチリの量
	//
	// 衝撃の数だけ回す計算をここで1テクセル1回に済ませ、
	// SceneVolumetricFogPass はレイの1歩ごとにこれを1回引くだけにする。
	//
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
		static constexpr int ROOT_CAMERA_CB = 0;
		static constexpr int ROOT_GROUND_FIELD_CB = 1;
		static constexpr int ROOT_OUTPUT_UAV = 2;
		static constexpr int ROOT_IMPULSE_SRV = 3;

		// パスが回り始めてからの経過時間(秒)。
		// 実行インスタンスごとに持つので、パイプラインを組み直すと 0 から数え直す
		float m_elapsedTime = 0.0f;
	};
}
