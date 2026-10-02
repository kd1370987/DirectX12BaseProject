#pragma once
#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// GroundVolumetricFogPass
	//
	// カメラから地面までのレイをマーチして、地面付近に立つフォグを書き出す。
	//   rgb = フォグの色 / a = フォグの濃さ(0..1)
	//
	// 濃さはグラウンドフィールド(GroundFieldPass の出力)で決まるので、
	// 衝撃が通ったところにだけ土煙のようなフォグが立つ。
	//
	// ノイズテクスチャはグラフのリソースではないので、スロットには乗らない。
	// アセットのGUIDを持っておき、番号を直接渡す(未設定ならノイズなしで一様に立つ)
	//======================================================================================
	class GroundVolumetricFogPass : public Pass
	{
	public:
		~GroundVolumetricFogPass() override = default;

		void SetupSlots() override;

		void Compile(const PassContext& a_context) override;
		void Update(const PassContext& a_context) override;


		void Archive(Engine::Persistence::Archive& a_arch) override;

		//----------------------------------------------------------------------------------
		// 編集対象の値 : エディターはここだけを触る
		//----------------------------------------------------------------------------------
		struct Params
		{
			// シェーダーへ送る調整値。time はパスが毎フレーム上書きする
			GroundFogCB cb = {};

			// ノイズテクスチャ : 未設定ならノイズなし
			Engine::GUID noiseTexGUID = {};
		};
		Params& RefParams() { return m_params; }

	private:

		// ルートパラメータの番号 : シェーダー(GroundVolumetricFogCS)の並びと合わせる
		static constexpr int kRootCameraCB = 0;
		static constexpr int kRootFogCB = 1;
		static constexpr int kRootInputSRV = 2;
		static constexpr int kRootOutputUAV = 3;
		static constexpr int kRootNoiseSRV = 4;

		// ノイズが張られていないときに渡す番号(シェーダーの NOISE_INDEX_NONE と合わせる)
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
