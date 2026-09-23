#pragma once
#include "../../CBData.h"

namespace Engine::Resource
{
	class Texture;
}

namespace Engine::Graphics
{
	//==========================================================================================
	// シーンの見え方
	//
	// パスが読む「カメラと、その周りの描画設定」をまとめて持つ。
	//   ・カメラ(ビュー/射影と、GPUへ送る形 : 転置・TAAのジッター・前フレームの行列)
	//   ・エディターカメラなどの割り込み
	//   ・カメラ発の画面効果(被写界深度 / ラジアルブラー / 魚眼レンズ)
	//   ・環境光と空
	//
	// 値を入れるのはアプリ側のシステム(CamSetShaderSystem / SceneAmbientObject など)、
	// 読むのはレンダーパス。GPUへ送る形を作るのは GraphicsEngine::Execute の中(UpdateGPUCameraData)
	//==========================================================================================
	class SceneView
	{
	public:

		// 描画解像度(ジッターの幅を決める)を受け取る
		void Init(UINT a_renderWidth, UINT a_renderHeight);

		// GPUへ送るカメラを確定する : 割り込みを当ててから、転置・ジッター込みの形を作る
		void UpdateGPUCameraData();

		// フレームの終わり : 今フレームだけの画面効果を落とす
		void EndFrame();

		//--------------------------------------------------------------------------------------------
		// カメラ
		//--------------------------------------------------------------------------------------------
		void SetCameraMat(const Math::Matrix& a_worldMat);
		void SetProjMat(const Math::Matrix& a_projMat);

		// GetCameraData    : GPUへ送る形(転置・ジッター込み)。パスはこちらを読む
		// GetCPUCameraData : SetCameraMat / SetProjMat で入れたままの行列
		const CameraData& GetCameraData() const;
		const CameraData& GetCPUCameraData() const;

		// カメラの割り込み(エディターカメラなど)
		// ECS側のカメラ設定は描画(PreDraw)の中で行われるため、
		// 単に SetCameraMat を先に呼んでも上書きされてしまう。
		// ここに積んでおくと、ECS側の設定が終わった後・GPUデータ作成の直前に適用される。
		void SetCameraOverride(const Math::Matrix& a_worldMat, const Math::Matrix& a_projMat);
		void ClearCameraOverride();

		// TAA用のジッターを掛けるか。設定の持ち主(オプション)から毎フレーム流し込んでもらう
		void SetJitterEnabled(bool a_isEnabled) { m_isJitterEnabled = a_isEnabled; }

		//--------------------------------------------------------------------------------------------
		// カメラ発の画面効果(被写界深度 / ラジアルブラー / 魚眼レンズ)
		//
		// どれもカメラの持ち物で、アクティブカメラのコンポーネントから
		// CamSetShaderSystem が毎フレーム詰める。速度に応じて動く値がここに乗る。
		//
		// パス側はアセットに保存した自分の値を既定として持っているので、
		// 「今フレーム、カメラから送られてきたか」を Is～Override() で見て、
		// 送られていればそちらを優先する。
		// 送られなかったフレームは EndFrame でフラグが落ちるので、
		// パスは自分の値へ戻る(効果が前フレームの値で固まらない)
		//--------------------------------------------------------------------------------------------
		void SetDoFData(const DoFOptionCB& a_data);
		const DoFOptionCB& GetDoFData() const;
		bool IsDoFOverride() const { return m_isDoFOverride; }

		void SetRadialBlurData(const RadialBlurOptionCB& a_data);
		const RadialBlurOptionCB& GetRadialBlurData() const;
		bool IsRadialBlurOverride() const { return m_isRadialBlurOverride; }

		void SetFishEyeData(const FishEyeOptionCB& a_data);
		const FishEyeOptionCB& GetFishEyeData() const;
		bool IsFishEyeOverride() const { return m_isFishEyeOverride; }

		//--------------------------------------------------------------------------------------------
		// 環境光と空
		//
		// どちらもシーンに置いた SceneAmbientObject の持ち物で、毎フレーム流し込まれる。
		// 空のテクスチャは所有せずハンドルだけ預かるので、置いた側が消えたら
		// 空のハンドルへ戻してもらう(空の間はスカイパスが何も描かない)。
		//--------------------------------------------------------------------------------------------
		void SetAmbientData(const AmbientData& a_data);
		const AmbientData& GetAmbientData() const;
		AmbientData& RefAmbientData();

		void SetSkyData(const SkyData& a_data);
		const SkyData& GetSkyData() const;
		SkyData& RefSkyData();

		void SetSkyTexture(const Handle<Resource::Texture>& a_handle);
		const Handle<Resource::Texture>& GetSkyTexture() const;

	private:

		// カメラをGPU用データに変換
		void CreateGPUCameraData();

	private:

		// 描画解像度(ジッターの幅に使う)
		UINT m_renderWidth = 0;
		UINT m_renderHeight = 0;

		// カメラデータ(CPU側の値と、GPUへ送る形)
		CameraData m_cbCamera = {};
		CameraData m_cbGPUCamera = {};

		// カメラの割り込み用
		bool m_isCameraOverride = false;
		Math::Matrix m_cameraOverrideWorldMat = Math::Matrix::Identity();
		Math::Matrix m_cameraOverrideProjMat = Math::Matrix::Identity();

		// TAA : ジッターを掛けるかと、モーションベクター用の前フレームの行列
		bool m_isJitterEnabled = true;
		Math::Matrix m_prevViewMat = {};
		Math::Matrix m_prevProjMat = {};
		Math::Matrix m_prevNonJitteredViewProj = {};
		int m_totalFrameCount = 0;

		// 環境データ
		AmbientData m_cbAmbient = {};

		// スカイの設定と、引くスカイテクスチャ(所有はしない)
		SkyData m_cbSky = {};
		Handle<Resource::Texture> m_skyTexHandle = {};

		// 被写界深度データ(アクティブカメラの FocusParamComponent から毎フレーム設定)
		DoFOptionCB m_cbDoF = {};

		// ラジアルブラーデータ(アクティブカメラの RadialBlurComponent から毎フレーム設定)
		RadialBlurOptionCB m_cbRadialBlur = {};

		// 魚眼レンズデータ(アクティブカメラの FishEyeComponent から毎フレーム設定)
		FishEyeOptionCB m_cbFishEye = {};

		// 今フレーム、カメラから画面効果の値が送られてきたか。
		// EndFrame で落とすので、送られなかったフレームはパス側の値が使われる
		bool m_isDoFOverride = false;
		bool m_isRadialBlurOverride = false;
		bool m_isFishEyeOverride = false;
	};
}
