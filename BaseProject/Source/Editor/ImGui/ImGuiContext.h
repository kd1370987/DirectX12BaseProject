#pragma once

#include "Editor/EditorCommon.h"

namespace Engine::Graphics::D3D12
{
	class DescriptorHeapManager;
}

namespace Editor
{
	class ImGuiContext
	{
	public:

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="a_hwnd">メインウィンドウハンドル</param>
		/// <param name="a_pHeapManager">ImGui用ヒープの持ち主(借り物)</param>
		bool Init(HWND a_hwnd, Graphics::D3D12::DescriptorHeapManager* a_pHeapManager);

		/// <summary>
		/// 解放
		/// </summary>
		void Release();

		// ImGui描画
		// ドックの土台はクライアント領域(ImGuiのメインビューポート)に合わせるため
		// サイズを外から渡す必要はない
		void Begin();
		void End(Graphics::D3D12::GraphicsCommandList* a_pCmdList);

	private:

		bool m_isInit = false;
	};
}