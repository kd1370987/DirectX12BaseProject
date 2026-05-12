#pragma once
namespace Engine::Editor::Helper
{
	void DrawMatrix(const DXSM::Matrix& a_mat);
	void DrawMatrixForPOS_ROT_SCALE(const std::string& a_name,DXSM::Matrix& a_mat);
}