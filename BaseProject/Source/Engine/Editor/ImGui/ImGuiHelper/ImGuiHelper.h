#pragma once
namespace Engine::Editor::Helper
{
	void DrawMatrix(DirectX::XMFLOAT4X4& a_mat);
	void DrawMatrixForPOS_ROT_SCALE(const std::string& a_name,DXSM::Matrix& a_mat);

	// クォータニオンンを引き込んでディグリーの
	void DragRotationDeg3FromQuaternion(DirectX::XMFLOAT4& a_quat);	


	/// <summary>
	/// SRVを画像としてImGui上に描画させる
	/// </summary>
	/// <param name="a_gpuHandle">GPUハンドル</param>
	/// <param name="a_width">横幅</param>
	/// <param name="a_height">縦</param>
	/// <return>実際に描画した範囲を返す</return>
	ImVec2 DrawSRVView(
		D3D12_GPU_DESCRIPTOR_HANDLE a_gpuHandle,
		float a_width, float a_height,
		float a_minSize = 100,float a_maxSize = 500
	);

	template<typename T>
	void DrawHandle(const Handle<T>& a_handle)
	{
		ImGui::Text("Handle : id = %d", static_cast<int>(a_handle.id));
		ImGui::Text("index = %d", static_cast<int>(a_handle.GetIndex()));
		ImGui::Text("generation = %d",static_cast<int>(a_handle.GetGeneration()));
	}

	template<typename T>
	void DrawHandle(const RangeHandle<T>& a_handle)
	{
		ImGui::Text("Handle : generation = %d", static_cast<int>(a_handle.generation));
		ImGui::Text("startIndex = %d", static_cast<int>(a_handle.startIndex));
		ImGui::Text("count = %d", static_cast<int>(a_handle.count));
	}
}

namespace Engine::Editor::Node
{
	// タイトルバー表示
	void TitleBar(const std::string& a_name);
}