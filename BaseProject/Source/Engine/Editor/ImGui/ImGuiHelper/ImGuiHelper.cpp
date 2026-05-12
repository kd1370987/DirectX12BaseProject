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
