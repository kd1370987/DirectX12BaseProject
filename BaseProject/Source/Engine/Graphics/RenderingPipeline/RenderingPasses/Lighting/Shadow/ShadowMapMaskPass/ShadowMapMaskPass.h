#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// ShadowMapMaskPass
	//
	// シャドウマップ(ShadowMapPass)と比べて、主光源の影マスクを作る。
	// 出力はレイトレの影と同じ形(1 = ひなた / 0 = 影)なので、ディファードはそのまま読める。
	//
	// 「描き足す」パスとして置く。
	// Shadow 入力にレイトレの影(のデノイズ後)を繋ぐと、同じリソースへ書く。
	//   影の求め方が ShadowMap   : シャドウマップの影で上書きする
	//   影の求め方が Raytracing : 何もしない(レイトレの影がそのまま流れる)
	// これでパイプラインを組み替えずに、シーンの設定だけで求め方を切り替えられる。
	//
	// デノイズの後ろに置くのは、シャドウマップの影は時間方向にためる必要がなく、
	// ためると動く物の影が尾を引くため。
	//
	// Shadow 入力を繋がずに単体で置いた場合は、どちらの設定でもシャドウマップの影を書く
	//======================================================================================
	class ShadowMapMaskPass : public Pass
	{
	public:
		~ShadowMapMaskPass() override = default;

		void SetupSlots() override;
		void OnLinksResolved() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

	private:

		// ルートパラメータの番号 : シェーダー(ShadowMapMaskCS)の並びと合わせる
		static constexpr int kRootCameraCB = 0;
		static constexpr int kRootShadowCB = 1;
		static constexpr int kRootInputSRV = 2;
		static constexpr int kRootOutputUAV = 3;
	};
}
