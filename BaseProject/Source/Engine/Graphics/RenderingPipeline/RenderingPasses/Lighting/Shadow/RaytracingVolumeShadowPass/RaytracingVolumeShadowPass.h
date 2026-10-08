#pragma once

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// RaytracingVolumeShadowPass
	//
	// ボリュメトリックフォグ用の影(VolumeShadow)をレイトレで作る。
	// 画素ごとにカメラから見えている面までの視線を歩き、各点から主光源へレイを飛ばして
	//   r = 日なたの割合(歩いた点の平均) / g = 歩いた範囲の終わり(カメラからの距離)
	// を書く。歩くのはシーンの影の距離(ShadowDistance)まで。
	// フォグ(SceneVolumetricFogPass)は、その範囲の平行光にこの割合を掛ける。
	//
	// ・フォグの影は低周波なので、低解像度(既定 1/4)で回す。
	//   フル解像度で 32 歩だと、面の影の 30 倍以上のレイを撃つことになる
	// ・可視判定だけのレイなので、DispatchRays ではなくインラインレイトレ(RayQuery)の
	//   コンピュートで飛ばす。PSO もルートシグネチャもグラフが張る普通のコンピュートパス
	// ・フォグもダストも濃さ 0 のフレームは歩かない(使う人が居ない)
	//
	// 影の求め方がシャドウマップのフレーム(LightManager::GetShadowMode)はレイを飛ばさず、
	// 出力を「影なし(範囲 0)」で埋めるだけにする。フォグはシャドウマップの影を引く
	//======================================================================================
	class RaytracingVolumeShadowPass : public Pass
	{
	public:
		~RaytracingVolumeShadowPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

		struct Params
		{
			// 視線を歩く歩数(1歩ごとにレイを1本)。0 なら作らない。
			// 1歩目の位置を毎フレームずらして TAA に均させるので、少なくても縞は残りにくい
			uint32_t stepCount = 8;

			// 描画解像度の何分の1で回すか(1 / 2 / 4 / 8)
			uint32_t resolutionDivisor = 4;
		};

		// 編集対象の値 : エディターはここだけを触る
		Params& RefParams() { return m_params; }

		// resolutionDivisor を出力スロットの大きさへ写す。変えたらグラフの組み直しが要る
		void ApplyResolution();

	private:

		// ルートパラメータの番号 : シェーダー(RayVolumeShadowCS)の並びと合わせる
		static constexpr int ROOT_CAMERA_CB = 0;
		static constexpr int ROOT_SUN_LIGHT_CB = 1;
		static constexpr int ROOT_PARAM_CB = 2;
		static constexpr int ROOT_INPUT_SRV = 3;
		static constexpr int ROOT_OUTPUT_UAV = 4;
		static constexpr int ROOT_TLAS = 5;

		// 歩く範囲と歩数
		// ※ HLSL 側(RayVolumeShadowCS の VolumeShadowParam)と並びを合わせること
		struct VolumeShadowCB
		{
			float distance;			// 歩く範囲(ビュー空間の奥行き。シーンの影の距離)
			uint32_t frame;			// 歩く位置のずらしをフレームごとに変える
			uint32_t stepCount;		// 視線を歩く歩数
			float pad;
		};

		// 出力を影なし(日なた 1 / 範囲 0)で埋める。フォグは平行光を遮らずに照らす
		void ClearOutput(const PassContext& a_context);

		Params m_params = {};

		// 回したフレームの数(歩く位置のずらしに使う)
		uint32_t m_frameCount = 0;
	};
}
