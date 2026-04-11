#pragma once

class RenderingEngine;
class DescriptorHeapManager;

class Log;
class Watch;

class RenderGraphView;
class ECSView;

class ComponentEdit;

class AssetResourceView;
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
		void Begin(UINT a_width,UINT a_height);
		void End(ID3D12GraphicsCommandList* a_pCmdList);

	private:

		bool m_isInit = false;
	};
}