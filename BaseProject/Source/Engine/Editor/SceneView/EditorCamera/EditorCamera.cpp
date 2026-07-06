#include "EditorCamera.h"

#include "../../../Input/InputManager/InputManager.h"

namespace Engine::Editor
{
	void EditorCamera::Init(UINT a_windowWidth, UINT a_windowHeight)
	{
		m_worldMat = DXSM::Matrix::Identity;
		m_aspectRatio = (float)a_windowWidth / (float)a_windowHeight;
		m_fov = 60.0f;
		m_nearZ = 0.1f;
		m_farZ = 1000.0f;

		m_projMat = DirectX::XMMatrixPerspectiveFovLH(
			DirectX::XMConvertToRadians(m_fov),
			m_aspectRatio,
			m_nearZ,
			m_farZ
		);
		m_projInvMat = m_projMat.Invert();
	}
	
	void EditorCamera::Update(float a_dt)
	{
		// ==========================================
		//  入力によるカメラ操作 (右クリックホールド中のみ)
		// ==========================================
		if (Input::InputManager::Instance().IsHold("FreeCamMode"))
		{
			// --- 回転処理 ---
			// マウスの移動量を取得
			float _mouseDeltaX = Input::InputManager::Instance().GetAxisState("Look").x;
			float _mouseDeltaY = Input::InputManager::Instance().GetAxisState("Look").y;

			float _rotSpeed = 0.003f; // マウス感度
			m_rot.y += _mouseDeltaX * _rotSpeed; // ヨー (左右回転)
			m_rot.x += _mouseDeltaY * _rotSpeed; // ピッチ (上下回転)

			// ジンバルロック防止のため、ピッチ(X回転)を -89度 ～ 89度 に制限
			float _limit = DirectX::XM_PIDIV2 - 0.01f;
			m_rot.x = std::clamp(m_rot.x, -_limit, _limit);

			// --- 移動処理 ---
			// 現在の回転から、カメラの正面・右・上のベクトルを計算する
			DXSM::Matrix _camRotMat = DXSM::Matrix::CreateRotationX(m_rot.x) * DXSM::Matrix::CreateRotationY(m_rot.y) * DXSM::Matrix::CreateRotationZ(m_rot.z);
			DXSM::Vector3 _forward = _camRotMat.Forward();
			DXSM::Vector3 _right = _camRotMat.Right();
			DXSM::Vector3 _up = _camRotMat.Up();

			float _moveSpeed = 10.0f * a_dt;

			// WASDキー等による移動 (Move軸の値 -1.0~1.0 を掛ける)
			DXSM::Vector2 _moveVec = Input::InputManager::Instance().GetAxisState("Move");

			// Y軸を前後、X軸を左右と想定
			m_pos += _forward * (_moveVec.y * _moveSpeed);
			m_pos += _right * (_moveVec.x * _moveSpeed);

			if (Input::InputManager::Instance().IsHold("FreeCamUp")) m_pos += _up * _moveSpeed;
			if (Input::InputManager::Instance().IsHold("FreeCamDown")) m_pos -= _up * _moveSpeed;
		}

		// ==========================================
		// 行列の更新
		// ==========================================
		DXSM::Matrix _scaleMat = DXSM::Matrix::CreateScale(m_scale);
		DXSM::Matrix _rotMat = DXSM::Matrix::CreateRotationX(m_rot.x) * DXSM::Matrix::CreateRotationY(m_rot.y) * DXSM::Matrix::CreateRotationZ(m_rot.z);
		DXSM::Matrix _transMat = DXSM::Matrix::CreateTranslation(m_pos);

		m_worldMat = _scaleMat * _rotMat * _transMat;
	}
}