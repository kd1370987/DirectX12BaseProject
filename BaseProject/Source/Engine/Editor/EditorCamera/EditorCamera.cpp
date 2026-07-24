#include "EditorCamera.h"

#include "../../Option/OptionManager.h"

namespace Engine::Editor
{
	void EditorCamera::Init()
	{
		BuildMatrix();
	}

	void EditorCamera::Update(float a_dt)
	{
		// 無効でも行列は作っておく(有効化した瞬間に不定な行列が使われないように)
		if (!m_isEnable)
		{
			m_isControlling = false;
			BuildMatrix();
			return;
		}

		//---------------------------------------------------------------
		// 操作の開始・終了判定
		//---------------------------------------------------------------
		// 右クリックを「押し始めた場所」がシーンビュー上の時だけ操作を開始する。
		// 一度始まったらカーソルが枠外へ出ても、離すまで継続する(Unityと同じ)。
		const bool _isRightDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);
		if (!m_isControlling)
		{
			// ギズモを操作中なら奪わない
			if (_isRightDown && m_isViewportHovered && !ImGuizmo::IsUsing())
			{
				m_isControlling = true;
			}
		}
		else if (!_isRightDown)
		{
			m_isControlling = false;
		}

		if (m_isControlling)
		{
			UpdateRotation();
			UpdateMove(a_dt);
		}

		BuildMatrix();
	}

	DXSM::Quaternion EditorCamera::CalcRotation() const
	{
		// Vector3 を取る CreateFromYawPitchRoll は引数の軸順が入れ替わるため、
		// スカラー版(yaw, pitch, roll)を明示的に使う
		DXSM::Quaternion _rot = DXSM::Quaternion::CreateFromYawPitchRoll(
			DirectX::XMConvertToRadians(m_yaw),
			DirectX::XMConvertToRadians(m_pitch),
			0.0f
		);
		_rot.Normalize();
		return _rot;
	}

	void EditorCamera::UpdateRotation()
	{
		const ImVec2 _delta = ImGui::GetIO().MouseDelta;
		if (_delta.x == 0.0f && _delta.y == 0.0f) return;

		m_yaw   += _delta.x * m_sensitivity;
		m_pitch += _delta.y * m_sensitivity;	// 下へ動かすと下を向く

		m_pitch = std::clamp(m_pitch, -MAX_PITCH, MAX_PITCH);

		// yaw は回し続けると際限なく増えるので畳んでおく
		if (m_yaw >  180.0f) m_yaw -= 360.0f;
		if (m_yaw < -180.0f) m_yaw += 360.0f;
	}

	void EditorCamera::UpdateMove(float a_dt)
	{
		ImGuiIO& _io = ImGui::GetIO();

		// テキスト入力中はキー入力を奪わない
		if (_io.WantTextInput) return;

		// ホイールで移動速度を調整する
		if (_io.MouseWheel != 0.0f)
		{
			m_moveSpeed *= std::pow(1.2f, _io.MouseWheel);
			m_moveSpeed = std::clamp(m_moveSpeed, 0.1f, 1000.0f);
		}

		// このエンジンは左手系でローカル +Z が前方
		const DXSM::Quaternion _rot = CalcRotation();
		const DXSM::Vector3 _forward = DXSM::Vector3::Transform(DXSM::Vector3(0.0f, 0.0f, 1.0f), _rot);
		const DXSM::Vector3 _right   = DXSM::Vector3::Transform(DXSM::Vector3(1.0f, 0.0f, 0.0f), _rot);

		DXSM::Vector3 _move = DXSM::Vector3::Zero;
		if (ImGui::IsKeyDown(ImGuiKey_W)) _move += _forward;
		if (ImGui::IsKeyDown(ImGuiKey_S)) _move -= _forward;
		if (ImGui::IsKeyDown(ImGuiKey_D)) _move += _right;
		if (ImGui::IsKeyDown(ImGuiKey_A)) _move -= _right;

		// 上下だけはワールドのY軸基準。カメラの傾きに引きずられない方が扱いやすい
		if (ImGui::IsKeyDown(ImGuiKey_E)) _move += DXSM::Vector3::Up;
		if (ImGui::IsKeyDown(ImGuiKey_Q)) _move -= DXSM::Vector3::Up;

		if (_move.LengthSquared() < 1e-8f) return;
		_move.Normalize();

		float _speed = m_moveSpeed;
		if (_io.KeyShift) _speed *= m_boostRate;

		m_pos += _move * _speed * a_dt;
	}

	void EditorCamera::BuildMatrix()
	{
		// カメラのワールド行列(GraphicsEngine 側で反転してビュー行列にされる)
		m_worldMat =
			DXSM::Matrix::CreateFromQuaternion(CalcRotation()) *
			DXSM::Matrix::CreateTranslation(m_pos);

		// 射影行列。アスペクトはウィンドウ設定から取る(ECS側のカメラと同じ作り方)
		const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
		const float _aspect = (_winOp.windowHegiht > 0)
			? static_cast<float>(_winOp.windowWidth) / static_cast<float>(_winOp.windowHegiht)
			: 16.0f / 9.0f;

		m_projMat = DirectX::XMMatrixPerspectiveFovLH(
			DirectX::XMConvertToRadians(m_fovY),
			_aspect,
			m_nearZ,
			m_farZ
		);
	}

	void EditorCamera::DrawEditUI()
	{
		ImGui::Checkbox("Enable", &m_isEnable);
		ImGui::TextDisabled("右ドラッグ中のみ操作 / WASD・EQ移動 / Shift加速 / ホイールで速度");

		ImGui::Separator();

		ImGui::DragFloat3("Position", &m_pos.x, 0.1f);
		ImGui::DragFloat("Yaw", &m_yaw, 0.5f);
		if (ImGui::DragFloat("Pitch", &m_pitch, 0.5f))
		{
			m_pitch = std::clamp(m_pitch, -MAX_PITCH, MAX_PITCH);
		}

		ImGui::Separator();

		ImGui::DragFloat("MoveSpeed", &m_moveSpeed, 0.1f, 0.1f, 1000.0f);
		ImGui::DragFloat("BoostRate", &m_boostRate, 0.1f, 1.0f, 100.0f);
		ImGui::DragFloat("Sensitivity", &m_sensitivity, 0.01f, 0.01f, 5.0f);

		ImGui::Separator();

		ImGui::DragFloat("FovY", &m_fovY, 0.5f, 1.0f, 179.0f);
		ImGui::DragFloat("NearZ", &m_nearZ, 0.01f, 0.001f, 100.0f);
		ImGui::DragFloat("FarZ", &m_farZ, 1.0f, 1.0f, 100000.0f);

		if (ImGui::Button("Reset"))
		{
			m_pos = { 0.0f, 3.0f, -10.0f };
			m_yaw = 0.0f;
			m_pitch = 0.0f;
			m_moveSpeed = 10.0f;
		}
	}
}
