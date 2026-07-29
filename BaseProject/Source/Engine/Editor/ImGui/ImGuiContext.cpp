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

		// ウィンドウが乗っているモニターの表示スケールを取得
		// DPI対応の有効化自体は NativeWindow::Create の中で
		// ウィンドウ作成前に済ませてある（後から呼んでも既存ウィンドウには効かない）
		float _mainScale = ImGui_ImplWin32_GetDpiScaleForHwnd(a_hwnd);
		if (_mainScale <= 0.0f) _mainScale = 1.0f;

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
		_style.ScaleAllSizes(_mainScale);		// 余白やウィジェットの大きさをモニターの表示スケールに合わせる
		_style.FontScaleDpi = _mainScale;		// フォントも同じ倍率で拡大する


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

	void ImGuiContext::Begin()
	{

		// ImGuiフレームの描画開始
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();		// ここで io.DisplaySize にクライアント領域のピクセル数が入る

		// 1論理ピクセルが何ピクセルのバックバッファに描かれるかを伝える
		//
		// バックバッファは描画解像度で作られ、スワップチェインの STRETCH で
		// クライアント領域まで引き伸ばされて表示される。
		// 一方でWin32バックエンドは io.DisplaySize にクライアント領域のサイズを入れ、
		// DX12バックエンドは DisplaySize * FramebufferScale をビューポートに使う。
		// ここを 1 のままにすると、UIはバックバッファの左上
		// (クライアント領域と同じピクセル数)だけに描かれ、
		// 右と下に余りを残した状態で画面全体へ伸ばされる = ウィンドウサイズと合わなくなる。
		//
		// 座標系はクライアント領域基準のまま(マウス座標もそのまま使える)で、
		// 描画とフォントのラスタライズだけバックバッファ解像度に合わせる。
		ImGuiIO& _io = ImGui::GetIO();
		const auto& _backBufferViewport = Engine::D3D12::D3D12Wrapper::Instance().GetViewport();
		if (_io.DisplaySize.x > 0.0f && _io.DisplaySize.y > 0.0f &&
			_backBufferViewport.Width > 0.0f && _backBufferViewport.Height > 0.0f)
		{
			_io.DisplayFramebufferScale = ImVec2(
				_backBufferViewport.Width / _io.DisplaySize.x,
				_backBufferViewport.Height / _io.DisplaySize.y
			);
		}

		ImGui::NewFrame();

		ImGuizmo::BeginFrame();

		// ドックの土台をクライアント領域全体に広げる
		//
		// 描画解像度(WindowOptionのwindowWidth/Height)を渡してはいけない。
		// ImGuiの座標系はクライアント領域基準なので、
		// 枠やDPI・ウィンドウモードでクライアント領域が変わった瞬間に
		// ドック領域だけが画面外へはみ出し、パネルのサイズが合わなくなる。
		const ImGuiViewport* _pViewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(_pViewport->WorkPos);
		ImGui::SetNextWindowSize(_pViewport->WorkSize);

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
