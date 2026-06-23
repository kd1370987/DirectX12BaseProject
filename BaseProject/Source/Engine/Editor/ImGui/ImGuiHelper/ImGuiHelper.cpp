#include "ImGuiHelper.h"

void Engine::Editor::Helper::DrawMatrixForPOS_ROT_SCALE(const std::string& a_name, DXSM::Matrix& a_mat)
{
	// 行列の分解
	DXSM::Vector3 _scale = {};
	DXSM::Quaternion _rotQuat = {};
	DXSM::Vector3 _pos = {};
	if (!a_mat.Decompose(_scale, _rotQuat, _pos))
	{
		ImGui::Text("Matrix Decompose Failed");
		return;
	}

	// クォータニオンをEulerに直す
	DXSM::Vector3 _rotRad = _rotQuat.ToEuler();

	// Degreeへ変換
	DXSM::Vector3 _rotDeg = {
		DirectX::XMConvertToDegrees(_rotRad.x),
		DirectX::XMConvertToDegrees(_rotRad.y),
		DirectX::XMConvertToDegrees(_rotRad.z)
	};

	// 表示
	if (ImGui::TreeNode(a_name.c_str()))
	{
		ImGui::Text("Position");
		ImGui::DragFloat3("##Position",&_pos.x,0.1f);

		ImGui::Separator();

		ImGui::Text("Rotation");
		ImGui::DragFloat3("##Rotation",&_rotDeg.x,0.5f);

		ImGui::Separator();

		ImGui::Text("Scale");
		ImGui::DragFloat3("##Scale",&_rotDeg.x,0.1f);

		ImGui::TreePop();
	}
}

void Engine::Editor::Helper::DragRotationDeg3FromQuaternion(DirectX::XMFLOAT4& a_quat)
{
	DXSM::Quaternion _quat = a_quat;
	DXSM::Vector3 _rotRad = _quat.ToEuler();
	// Degreeへ変換
	DXSM::Vector3 _rotDeg = {
		DirectX::XMConvertToDegrees(_rotRad.x),
		DirectX::XMConvertToDegrees(_rotRad.y),
		DirectX::XMConvertToDegrees(_rotRad.z)
	};

	ImGui::Text("Rotation");
	// 値が変更されたか取得
	if (ImGui::DragFloat3("##Rotation", &_rotDeg.x, 0.5f))
	{
		// Degree → Radian
		_rotRad = {
			DirectX::XMConvertToRadians(_rotDeg.x),
			DirectX::XMConvertToRadians(_rotDeg.y),
			DirectX::XMConvertToRadians(_rotDeg.z)
		};

		// Euler → Quaternion
		DXSM::Quaternion _newQuat =
			DXSM::Quaternion::CreateFromYawPitchRoll(
				_rotRad.y, // Yaw
				_rotRad.x, // Pitch
				_rotRad.z  // Roll
			);

		// 引数へ反映
		a_quat = {
			_newQuat.x,
			_newQuat.y,
			_newQuat.z,
			_newQuat.w
		};
	}
	
}

void Engine::Editor::Helper::DrawSRVView(D3D12_GPU_DESCRIPTOR_HANDLE a_gpuHandle, float a_width, float a_height, float a_minSize, float a_maxSize)
{
	ImTextureID _imTex = (ImTextureID)(a_gpuHandle.ptr);
	ImVec2 size = ImGui::GetContentRegionAvail();

	float aspect = a_width / a_height;

	if (size.x / size.y > aspect)
	{
		size.x = size.y * aspect;
	}
	else
	{
		size.y = size.x / aspect;
	}

	ImGui::Image(_imTex, size);
}

void Engine::Editor::Node::TitleBar(const std::string& a_name)
{
	ImNodes::BeginNodeTitleBar();
	ImGui::Text("%s", a_name.c_str());
	ImNodes::EndNodeTitleBar();
}
