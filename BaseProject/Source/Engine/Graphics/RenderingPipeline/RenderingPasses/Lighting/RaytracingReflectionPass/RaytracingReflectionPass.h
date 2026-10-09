#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// RaytracingReflectionPass
	//
	// 鏡面反射をレイトレで求める。
	// 画素ごとに視線を法線で反射したレイを1本飛ばし、当たった先の放射輝度を書く。
	//   rgb = 反射先の放射輝度(HDR) / a = 1 : 物に当たった / 0 : 空(環境)
	// 背景の画素とレイを飛ばさないフレームは (0, 0, 0, 0)。
	// 混ぜ方(フレネル・粗さ)は受け取る側(DeferredLightingPass の Reflection 入力)が決める。
	//
	// ・出力は 1920x1080 固定。描画解像度とは独立しているので、
	//   受け取る側は画素番号ではなく UV で引くこと
	// ・DispatchRays ではなくインラインレイトレ(RayQuery)のコンピュートで飛ばす。
	//   当たった先のシェーディングもシェーダーの中で済ませるので、
	//   PSO もルートシグネチャもグラフが張る普通のコンピュートパス
	// ・当たった先の照明は 主光源 + 環境光 + 自己発光(RayReflectionCS を参照)
	//======================================================================================
	class RaytracingReflectionPass : public Pass
	{
	public:
		~RaytracingReflectionPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

		// 出力の大きさ(固定)
		static constexpr UINT OUTPUT_WIDTH = 1920;
		static constexpr UINT OUTPUT_HEIGHT = 1080;

		struct Params
		{
			// レイの届く距離。これより遠いものは映らず、空の色になる
			float maxDistance = 1000.0f;

			// 当たった先で主光源への影を求めるか(レイがもう1本増える)
			bool isShadow = true;
		};

		// 編集対象の値 : エディターはここだけを触る
		Params& RefParams() { return m_params; }

	private:

		// ルートパラメータの番号 : シェーダー(RayReflectionCS)の並びと合わせる
		static constexpr int ROOT_CAMERA_CB = 0;
		static constexpr int ROOT_SUN_LIGHT_CB = 1;
		static constexpr int ROOT_SKY_CB = 2;
		static constexpr int ROOT_AMBIENT_CB = 3;
		static constexpr int ROOT_PARAM_CB = 4;
		static constexpr int ROOT_INPUT_SRV = 5;
		static constexpr int ROOT_BUFFER_SRV = 6;
		static constexpr int ROOT_OUTPUT_UAV = 7;
		static constexpr int ROOT_TLAS = 8;

		// 反射の設定
		// ※ HLSL 側(RayReflectionCS の ReflectionParam)と並びを合わせること
		struct ReflectionCB
		{
			float maxDistance;		// レイの届く距離
			uint32_t isShadow;		// 当たった先で主光源への影を求めるか
			float pad[2];
		};

		// 出力を「反射なし」(0, 0, 0, 0)で埋める
		void ClearOutput(const PassContext& a_context);

		Params m_params = {};
	};
}
