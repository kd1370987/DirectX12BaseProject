#pragma once

#include "../RenderingPipeline.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// UpScalePass
	//
	// ハーフ解像度で作ったGIを、深度と法線を手がかりにフル解像度へ引き伸ばす。
	// 単純な拡大だとエッジがにじむので、エッジを見ながら重みを決める
	//======================================================================================
	class UpScalePass : public Pass
	{
	public:
		~UpScalePass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;

		EPassEditResult EditUpdate() override;
		void EditNode() override;

		void Archive(Engine::Persistence::Archive& a_arch) override;

	private:

		// 入力と出力の実サイズから倍率を求める
		float CalcScaleRatio(const PassContext& a_context) const;

	private:

		//----------------------------------------------------------------------------------
		// シェーダーへ送る調整値
		//
		// ※ HLSL 側と並びを合わせること。
		//    移植時にここを別物(phiDepth/phiNormal)にしてしまい、
		//    scaleRatio へ 1.0 が入ってハーフ解像度のGIを等倍で読んでいた
		//----------------------------------------------------------------------------------
		struct UpScaleCB
		{
			float scaleRatio;	// 入力に対する出力の倍率。実際の解像度から毎フレーム求める
			float depthSigma;	// ビュー深度に対する相対値(0.05 = 距離の5%まで同じ面とみなす)
			float normalPower;	// pow()の指数。小さいとエッジ判定がほぼ効かない
			float pad;
		};
		UpScaleCB m_cb = { 2.0f, 0.05f, 32.0f, 0.0f };
	};
}
