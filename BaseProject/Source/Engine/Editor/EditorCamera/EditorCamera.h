#pragma once

namespace Engine::Editor
{
	//=======================================================================
	//
	// エディター専用のフリーカメラ
	//
	// Unity / Unreal と同じく、右クリックを押している間だけ操作できる。
	//   右ドラッグ : 視点回転
	//   W / A / S / D : 前後左右
	//   E / Q         : 上昇 / 下降(ワールドのY軸基準)
	//   Shift         : 加速
	//   ホイール      : 移動速度の調整
	//
	// 操作の開始判定はシーンビュー上にカーソルがある時だけ。
	// 一度始まったらカーソルが枠外へ出ても、ボタンを離すまで操作を続ける。
	//
	//=======================================================================
	class EditorCamera
	{
	public:

		void Init();

		// a_dt : 実フレーム時間
		void Update(float a_dt);

		//-------------------------------------------------------------------
		// 行列取得 : GraphicsEngine へ割り込ませるために使う
		//-------------------------------------------------------------------
		// カメラのワールド行列(ビュー行列ではない)
		const DXSM::Matrix& GetWorldMatrix() const { return m_worldMat; }
		const DXSM::Matrix& GetProjMatrix()  const { return m_projMat; }

		//-------------------------------------------------------------------
		// 状態
		//-------------------------------------------------------------------
		// 無効にすると ECS 側のカメラがそのまま使われる
		bool IsEnable() const { return m_isEnable; }
		void SetEnable(bool a_isEnable) { m_isEnable = a_isEnable; }

		// 右クリック操作中かどうか(ギズモなど他の操作との競合を避ける用)
		bool IsControlling() const { return m_isControlling; }

		// シーンビューにカーソルが乗っているか。SceneViewPanel が毎フレーム設定する。
		void SetViewportHovered(bool a_isHovered) { m_isViewportHovered = a_isHovered; }

		// パラメーター調整用UI
		void DrawEditUI();

	private:

		// 現在の姿勢からクォータニオンを作る
		DXSM::Quaternion CalcRotation() const;

		void UpdateRotation();
		void UpdateMove(float a_dt);
		void BuildMatrix();

	private:

		// 姿勢
		DXSM::Vector3 m_pos = { 0.0f, 3.0f, -10.0f };
		float m_yaw   = 0.0f;	// 度
		float m_pitch = 0.0f;	// 度

		// 操作感
		float m_moveSpeed   = 10.0f;	// メートル/秒
		float m_boostRate   = 4.0f;		// Shift を押している間の倍率
		float m_sensitivity = 0.15f;	// 度/ピクセル

		// 射影
		float m_fovY  = 60.0f;
		float m_nearZ = 0.1f;
		float m_farZ  = 1000.0f;

		// 状態
		bool m_isEnable         = true;
		bool m_isControlling    = false;
		bool m_isViewportHovered = false;

		// 出力
		DXSM::Matrix m_worldMat = DXSM::Matrix::Identity;
		DXSM::Matrix m_projMat  = DXSM::Matrix::Identity;

		// 真上・真下を向くと視線とUpベクトルが平行になり行列が破綻するため手前で止める
		static constexpr float MAX_PITCH = 89.0f;
	};
}
