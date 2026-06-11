#pragma once
namespace Engine::Editor::Helper
{
	void DrawMatrix(const DXSM::Matrix& a_mat);
	void DrawMatrixForPOS_ROT_SCALE(const std::string& a_name,DXSM::Matrix& a_mat);

	// クォータニオンンを引き込んでディグリーの
	void DragRotationDeg3FromQuaternion(DirectX::XMFLOAT4& a_quat);	
}

namespace Engine::Editor::Node
{
	// タイトルバー表示
	void TitleBar(const std::string& a_name);
}