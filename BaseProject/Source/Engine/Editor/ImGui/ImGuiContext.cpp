#include "ImGuiContext.h"

#include "Engine/D3D12/D3D12Wrapper/RenderingEngine.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

void ImGuiContex::Init(HWND a_hwnd)
{
	auto& _pD3DWrapper = RenderingEngine::Instance();
	auto& _pDescriptorManager = DescriptorHeapManager::Instance();

	// メインモニターのスケールとDPI作成
	ImGui_ImplWin32_EnableDpiAwareness();
	float _mainScale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{0,0},MONITOR_DEFAULTTOPRIMARY));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& _io = ImGui::GetIO(); (void)_io;
	_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;		// キーボードを使用可能に
	_io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;		// ゲームパッドを使用可能に

	// ImGuiのセットアップ
	ImGui::StyleColorsDark();

	// サイズのセットアップ
	ImGuiStyle& _style = ImGui::GetStyle();
	_style.ScaleAllSizes(_mainScale);		// 取得したモニターサイズと合わせる
	_style.FontScaleDpi = _mainScale;		// 数値としても記録


	// 描画するバックエンド・プラットフォームを設定
	ImGui_ImplWin32_Init(a_hwnd);

	// DX12オブジェクトをセット
	ImGui_ImplDX12_InitInfo _initInfo = {};
	_initInfo.Device = _pD3DWrapper.GetDevice();
	_initInfo.CommandQueue = _pD3DWrapper.GetCommandQueue();
	_initInfo.NumFramesInFlight = static_cast<int>(CPU_FRAME_COUNT);
	_initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	_initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
	_initInfo.SrvDescriptorHeap = _pDescriptorManager.GetCBV_SRV_UAVHeap();
	_initInfo.LegacySingleSrvCpuDescriptor = _pDescriptorManager.GetImGuiCPUHandle();
	_initInfo.LegacySingleSrvGpuDescriptor = _pDescriptorManager.GetImGuiGPUHandle();
	ImGui_ImplDX12_Init(&_initInfo);


}

void ImGuiContex::CallImGuiDrawData(ID3D12GraphicsCommandList* a_pCmdList)
{
	// ImGuiフレームの描画開始
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 任意のimgui関数の呼び出し
	ImGui::Begin("Hello, world");
	ImGui::End();

	ImGui::Render();

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),a_pCmdList);
}

void ImGuiContex::Release()
{
	// メモリの解放
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
