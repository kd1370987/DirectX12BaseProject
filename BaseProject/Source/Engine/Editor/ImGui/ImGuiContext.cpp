#include "ImGuiContext.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "Log/Log.h"
#include "Watch/Watch.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
namespace Engine::Editor
{
	bool ImGuiContext::Init(HWND a_hwnd)
	{
		auto& _pD3DWrapper = Engine::D3D12::D3D12Wrapper::Instance();
		auto& _pDescriptorManager = D3D12::DescriptorHeapManager::Instance();

		// メインモニターのスケールとDPI作成
		ImGui_ImplWin32_EnableDpiAwareness();
		float _mainScale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0,0 }, MONITOR_DEFAULTTOPRIMARY));

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& _io = ImGui::GetIO();
		(void)_io;
		_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;		// キーボードを使用可能に
		_io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;		// ゲームパッドを使用可能に
		_io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;			// ImGuiDockingの有効化

		// 日本語対応
		ImFontConfig _config;
		_config.MergeMode = true;
		_io.Fonts->AddFontDefault();
		_io.Fonts->AddFontFromFileTTF(
			"c:\\Windows\\Fonts\\msgothic.ttc",
			13.0f, 
			&_config,
			_io.Fonts->GetGlyphRangesJapanese()
		);

		// ImGuiのセットアップ
		ImGui::StyleColorsDark();

		// サイズのセットアップ
		ImGuiStyle& _style = ImGui::GetStyle();
		_style.ScaleAllSizes(_mainScale);		// 取得したモニターサイズと合わせる
		_style.FontScaleDpi = _mainScale;		// 数値としても記録
		_style.ScaleAllSizes(1.0f);				// 全体的に縮小


		// 描画するバックエンド・プラットフォームを設定
		ImGui_ImplWin32_Init(a_hwnd);

		// DX12オブジェクトをセット
		ImGui_ImplDX12_InitInfo _initInfo = {};
		_initInfo.Device = _pD3DWrapper.GetDevice();
		_initInfo.CommandQueue = _pD3DWrapper.GetCommandQueue();
		_initInfo.NumFramesInFlight = static_cast<int>(CPU_FRAME_COUNT);
		_initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		_initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
		_initInfo.SrvDescriptorHeap = _pDescriptorManager.GetImGuiHeap();
		_initInfo.LegacySingleSrvCpuDescriptor = _pDescriptorManager.GetImGuiCPUHandle();
		_initInfo.LegacySingleSrvGpuDescriptor = _pDescriptorManager.GetImGuiGPUHandle();
		ImGui_ImplDX12_Init(&_initInfo);

		// ノードエディター
		ImNodes::CreateContext();
		return true;
	}

	void ImGuiContext::Begin(UINT a_width, UINT a_height)
	{
	
		// ImGuiフレームの描画開始
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGuizmo::BeginFrame();

		// ウィンドウサイズをゲーム画面に合わせる
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(a_width, a_height));

		// ビューポート切り替え
		ImGui::Begin(
			"MainDockWindow",
			nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_NoDocking
		);
		{
			// ベース
			ImGuiID _dockSpaceID = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(_dockSpaceID, ImGui::GetContentRegionAvail(), ImGuiDockNodeFlags_PassthruCentralNode);
		}

		ImGui::End();
	}

	void ImGuiContext::End(D3D12::GraphicsCommandList * a_pCmdList)
	{
		// ImGui描画
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), a_pCmdList);
	}

	void ImGuiContext::Release()
	{
		// ノード
		ImNodes::DestroyContext();

		// メモリの解放
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
}
