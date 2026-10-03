#pragma once
#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Graphics::Pipeline
{
	//======================================================================================
	// GroundVolumetricFogPass
	//
	// 地面メッシュから一定の高さ(fogHeight)まで漂うチリを、レイマーチで書き出す。
	//   rgb = フォグの色 / a = フォグの濃さ(0..1)
	//
	// チリの層は常にあり、高さはその画素で見えている地面から測る(画面空間の近似)。
	// 衝撃(GroundImpulse)が来ると、波が通り過ぎた内側のチリが払われ、
	// 波頭に寄せられて巻き上がる。時間が経つと払った場所へ戻る(式は GroundFieldCS と共通)。
	//
	// 衝撃の配列とノイズテクスチャはグラフのリソースではないので、スロットには乗らない。
	// 衝撃は GraphicsEngine が詰めたものの番号を、
	// ノイズはアセットのGUIDを持っておき読み込んだものの番号を直接渡す(未設定ならノイズなし)
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
			// シェーダーへ送る調整値。time と impulseCount はパスが毎フレーム上書きする
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
		static constexpr int kRootImpulseSRV = 5;

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
