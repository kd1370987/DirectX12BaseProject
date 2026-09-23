#include "SceneView.h"

namespace Engine::Graphics
{
	void SceneView::Init(UINT a_renderWidth, UINT a_renderHeight)
	{
		m_renderWidth = a_renderWidth;
		m_renderHeight = a_renderHeight;

		// 環境光は、シーンが流し込むまで無し
		m_cbAmbient = {};
		m_cbAmbient.ambientColorScale = { 0,0,0 };
	}

	//==========================================================================================
	// GPUへ送るカメラを確定する(Execute の中、描画要求を積み終えた後に1回)
	//
	// ECS側のカメラ設定は描画(PreDraw)の中で行われるので、
	// エディターカメラなどの割り込みは必ずその後・GPUデータを作る直前に当てる
	//==========================================================================================
	void SceneView::UpdateGPUCameraData()
	{
		if (m_isCameraOverride)
		{
			SetCameraMat(m_cameraOverrideWorldMat);
			SetProjMat(m_cameraOverrideProjMat);
		}

		CreateGPUCameraData();
	}

	//==========================================================================================
	// フレームの終わり
	//
	// 画面効果は毎フレーム、アクティブカメラが設定し直す。
	// ここで落としておけば、カメラが居ない/その設定を持たないフレームは
	// 前フレームの値で効き続けることがない。
	// フラグを下ろすと、パスはアセットに保存した自分の値へ戻る
	//==========================================================================================
	void SceneView::EndFrame()
	{
		m_cbDoF = {};
		m_isDoFOverride = false;

		m_cbRadialBlur = {};
		m_isRadialBlurOverride = false;

		m_cbFishEye = {};
		m_isFishEyeOverride = false;
	}

	void SceneView::SetCameraMat(const Math::Matrix& a_worldMat)
	{
		// 座標を代入
		m_cbCamera.pos = { a_worldMat._41,a_worldMat._42,a_worldMat._43 ,1 };

		// ビュー行列・逆ビュー行列をセット
		m_cbCamera.viewMat = a_worldMat.Invert();
		m_cbCamera.viewInvMat = a_worldMat;
	}
	void SceneView::SetProjMat(const Math::Matrix& a_projMat)
	{
		m_cbCamera.projMat = a_projMat;
		m_cbCamera.projInvMat = a_projMat.Invert();
	}
	void SceneView::SetDoFData(const DoFOptionCB& a_data)
	{
		m_cbDoF = a_data;

		// 今フレームはカメラが決めた値を使う
		m_isDoFOverride = true;
	}
	const DoFOptionCB& SceneView::GetDoFData() const
	{
		return m_cbDoF;
	}
	void SceneView::SetRadialBlurData(const RadialBlurOptionCB& a_data)
	{
		m_cbRadialBlur = a_data;

		// 今フレームはカメラが決めた値を使う
		m_isRadialBlurOverride = true;
	}
	const RadialBlurOptionCB& SceneView::GetRadialBlurData() const
	{
		return m_cbRadialBlur;
	}
	void SceneView::SetFishEyeData(const FishEyeOptionCB& a_data)
	{
		m_cbFishEye = a_data;

		// 今フレームはカメラが決めた値を使う
		m_isFishEyeOverride = true;
	}
	const FishEyeOptionCB& SceneView::GetFishEyeData() const
	{
		return m_cbFishEye;
	}
	void SceneView::SetCameraOverride(const Math::Matrix& a_worldMat, const Math::Matrix& a_projMat)
	{
		m_isCameraOverride = true;
		m_cameraOverrideWorldMat = a_worldMat;
		m_cameraOverrideProjMat = a_projMat;
	}
	void SceneView::ClearCameraOverride()
	{
		m_isCameraOverride = false;
	}
	const CameraData& SceneView::GetCameraData() const
	{
		return m_cbGPUCamera;
	}
	const CameraData& SceneView::GetCPUCameraData() const
	{
		return m_cbCamera;
	}
	void SceneView::SetAmbientData(const AmbientData& a_data)
	{
		m_cbAmbient = a_data;
	}
	const AmbientData& SceneView::GetAmbientData() const
	{
		return m_cbAmbient;
	}
	AmbientData& SceneView::RefAmbientData()
	{
		return m_cbAmbient;
	}
	void SceneView::SetSkyData(const SkyData& a_data)
	{
		m_cbSky = a_data;
	}
	const SkyData& SceneView::GetSkyData() const
	{
		return m_cbSky;
	}
	SkyData& SceneView::RefSkyData()
	{
		return m_cbSky;
	}
	void SceneView::SetSkyTexture(const Handle<Resource::Texture>& a_handle)
	{
		m_skyTexHandle = a_handle;
	}
	const Handle<Resource::Texture>& SceneView::GetSkyTexture() const
	{
		return m_skyTexHandle;
	}

	void SceneView::CreateGPUCameraData()
	{
		// リセット
		m_cbGPUCamera = {};

		// ジッターオフセット計算
		float _jitterX = 0.0f;
		float _jitterY = 0.0f;

		// ジッターオンオフ(SetJitterEnabled で切り替え。OFFならジッター0でTAAはブレンドのみ)
		if (m_isJitterEnabled && m_renderWidth > 0 && m_renderHeight > 0)
		{
			// ハルトンシーケンスのテーブル（ピクセル中心地からのオフセット値 -0.5f ～ 0.5f）
			static const float _sHaltonX[16] = {
				0.000000f, -0.250000f,  0.250000f, -0.375000f,
				0.125000f, -0.125000f,  0.375000f, -0.437500f,
				0.062500f, -0.187500f,  0.312500f, -0.312500f,
				0.187500f, -0.062500f,  0.437500f, -0.468750f
			};
			static const float _sHaltonY[16] = {
				0.000000f,  0.166667f, -0.166667f,  0.500000f,
			   -0.500000f, -0.277778f,  0.055556f,  0.388889f,
			   -0.388889f, -0.055556f,  0.277778f,  0.444444f,
			   -0.222222f,  0.111111f, -0.444444f,  0.222222f
			};
			uint32_t _sampleIndex = m_totalFrameCount % 16;

			// プロジェクション空間（NDC）のサイズに変換 : NDCは幅が２(-1～1)だから2倍
			_jitterX = (_sHaltonX[_sampleIndex] / static_cast<float>(m_renderWidth)) * 2.0f;
			_jitterY = (_sHaltonY[_sampleIndex] / static_cast<float>(m_renderHeight)) * 2.0f;
		}

		// カメラの行列を一時的に取得
		Math::Matrix _viewMat = m_cbCamera.viewMat;
		Math::Matrix _projMat = m_cbCamera.projMat;
		Math::Matrix _invViewMat = m_cbCamera.viewInvMat;
		Math::Matrix _invProjMat = m_cbCamera.projInvMat;

		// モーションベクター用のジッターなしViewProjを計算
		Math::Matrix _nonJitteredViewProj = _viewMat * _projMat;
		Math::Matrix _nonJitteredInvViewProj = _nonJitteredViewProj.Invert();

		// 描画用のジッターあり投影行列を作成
		Math::Matrix _jitteredProjMat = _projMat;
		_jitteredProjMat._31 += _jitterX;
		_jitteredProjMat._32 += _jitterY;

		// 描画用のジッターありViewProjとその逆行列を計算
		Math::Matrix _jitteredViewProj = _viewMat * _jitteredProjMat;
		Math::Matrix _invJitteredProj = _jitteredProjMat.Invert();
		Math::Matrix _invJitteredViewProj = _jitteredViewProj.Invert();

		// GPU転送用バッファへの詰め込み
		m_cbGPUCamera.pos = m_cbCamera.pos;

		// 通常の描画（SV_Positionの計算）にはジッターありを使う
		m_cbGPUCamera.viewMat = _viewMat.Transpose();
		m_cbGPUCamera.projMat = _jitteredProjMat.Transpose();
		m_cbGPUCamera.viewInvMat = _invViewMat.Transpose();
		m_cbGPUCamera.projInvMat = _invJitteredProj.Transpose();
		m_cbGPUCamera.viewProjMat = _jitteredViewProj.Transpose();
		m_cbGPUCamera.invViewProjMat = _invJitteredViewProj.Transpose();

		// モーションベクターの計算にはジッターなしを使う
		m_cbGPUCamera.nonJitteredProj = _projMat.Transpose();
		m_cbGPUCamera.nonJitteredViewProj = _nonJitteredViewProj.Transpose();
		m_cbGPUCamera.nonJitteredInvViewProj = _nonJitteredInvViewProj.Transpose();

		// 過去フレームのジッターなし行列の処理
		m_cbGPUCamera.prevView = m_prevViewMat.Transpose();
		m_cbGPUCamera.prevProj = m_prevProjMat.Transpose();
		m_cbGPUCamera.prevViewProj = m_prevNonJitteredViewProj.Transpose();

		// 次のフレームのためにジッターなしデータを保存
		m_prevViewMat = _viewMat;
		m_prevProjMat = _projMat;
		m_prevNonJitteredViewProj = _nonJitteredViewProj;

		// 完成したデータから、フラスタム平面を求める
		m_cbGPUCamera.ExtractFrustumPlanes(_nonJitteredViewProj);

		// フレームカウントを進める
		m_totalFrameCount++;
	}
}
