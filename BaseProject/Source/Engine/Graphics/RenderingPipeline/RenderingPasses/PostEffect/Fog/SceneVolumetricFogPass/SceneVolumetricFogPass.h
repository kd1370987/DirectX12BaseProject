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
	// ダストの高さはレイの終点の真下の地面から測る(GroundDepthPass の地面だけの深度から探す)。
	// 衝撃でチリが払われる・波頭へ寄せられる量は GroundFieldPass が書いたテクスチャを引く。
	// 地面の深度を繋がなければダストは出ず、フィールドを繋がなければチリは衝撃で動かない。
	//
	// ノイズテクスチャはグラフのリソースではないので、スロットには乗らない。
	// アセットのGUIDを持っておき、読み込んだものの番号を直接渡す(未設定ならノイズなし)
	//======================================================================================
	class SceneVolumetricFogPass : public Pass
	{
	public:
		~SceneVolumetricFogPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

		//----------------------------------------------------------------------------------
		// 編集対象の値 : エディターはここだけを触る
		//----------------------------------------------------------------------------------
		struct Params
		{
			// シーンのフォグ
			SceneFogCB sceneFog = {};

			// グラウンドダスト。time はパスが毎フレーム上書きする
			GroundDustCB groundDust = {};

			// ノイズテクスチャ(ダストに掛ける) : 未設定ならノイズなし
			Engine::GUID noiseTexGUID = {};
		};
		Params& RefParams() { return m_params; }

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

		Params m_params = {};

		// 読み込んだノイズテクスチャ。
		// GUID はエディターから差し替わるので、読んだときの GUID を控えて食い違ったら読み直す
		ResourceRef<Resource::Texture> m_noiseTexRef = {};
		Engine::GUID m_loadedNoiseGUID = {};

		// パスが回り始めてからの経過時間(秒)。
		// 実行インスタンスごとに持つので、パイプラインを組み直すと 0 から数え直す
		float m_elapsedTime = 0.0f;
	};
}
