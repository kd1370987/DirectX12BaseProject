#pragma once
#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// SceneVolumetricFogPass
	//
	// カメラから見えている面(空なら maxDistance 先)までのレイに沿って、2つの媒質を積分する。
	//   ・シーンのフォグ   : シーン全体に一様に漂う(SceneFogCB)
	//   ・グラウンドダスト : 地面から一定の高さまで漂うチリ(GroundDustCB)
	// 出力 : rgb = フォグの色(2つの媒質の色を濃さで混ぜたもの) / a = フォグの濃さ(0..1)
	//
	// ダストは見えている面が地面そのものの画素にだけ置く(GroundDepthPass の地面だけの深度と比べる)。
	// 手前に物体がある画素・空の画素はシーンのフォグだけ。
	// 衝撃でチリが払われる・波頭へ寄せられる量は GroundFieldPass が書いたテクスチャを引く。
	// 地面の深度を繋がなければダストは出ず、フィールドを繋がなければチリは衝撃で動かない。
	//
	// 値(フォグ・ダストの調整値とノイズテクスチャ)はシーン(Engine::Scene::SceneAmbient)の
	// 持ち物で、パスは SceneView から受け取って送るだけ。パス自身は設定を持たない。
	// ノイズテクスチャはグラフのリソースではないので、スロットには乗らない。
	// シーンが読み込んだものの番号を直接渡す(未設定・読み込み中ならノイズなし)
	//======================================================================================
	class SceneVolumetricFogPass : public Pass
	{
	public:
		~SceneVolumetricFogPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

	private:

		// ルートパラメータの番号 : シェーダー(SceneVolumetricFogCS)の並びと合わせる
		static constexpr int kRootCameraCB = 0;
		static constexpr int kRootSceneFogCB = 1;
		static constexpr int kRootGroundDustCB = 2;
		static constexpr int kRootInputSRV = 3;
		static constexpr int kRootOutputUAV = 4;
		static constexpr int kRootNoiseSRV = 5;

		// ノイズが張られていないときに渡す番号(シェーダーの DESCRIPTOR_INDEX_NONE と合わせる)
		static constexpr UINT kNoiseIndexNone = 0xFFFFFFFF;

		// パスが回り始めてからの経過時間(秒)。
		// 実行インスタンスごとに持つので、パイプラインを組み直すと 0 から数え直す
		float m_elapsedTime = 0.0f;
	};
}
