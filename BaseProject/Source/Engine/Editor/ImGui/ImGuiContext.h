#pragma once

namespace Engine::Editor
{
	class ImGuiContext
	{
	public:

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="a_hwnd">メインウィンドウハンドル</param>
		bool Init(HWND a_hwnd);

		/// <summary>
		/// 解放
		/// </summary>
		void Release();

		// ImGui描画
		// ドックの土台はクライアント領域(ImGuiのメインビューポート)に合わせるため
		// サイズを外から渡す必要はない
		void Begin();
		void End(D3D12::GraphicsCommandList* a_pCmdList);

	private:

		bool m_isInit = false;
	};
}