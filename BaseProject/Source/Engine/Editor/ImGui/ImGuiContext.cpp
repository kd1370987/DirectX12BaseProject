#include "ImGuiContext.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "Log/Log.h"
#include "Watch/Watch.h"

bool ImGuiContex::Init(HWND a_hwnd)
{
	auto& _pD3DWrapper = D3D12Wrapper::Instance();
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

	// ログ
	if(!m_upLog)
	{
		m_upLog = std::make_unique<Log>();
		m_upLog->Init();
	}

	return true;
}

void ImGuiContex::CallImGuiDrawData(ID3D12GraphicsCommandList* a_pCmdList)
{
	// ImGuiフレームの描画開始
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// ログ表示
	m_upLog->Draw("Log");

	// 計測表示
	for (auto& [_name, _watch] : m_upWatchMap)
	{
		_watch->DrawResult(_name);
	}

	ImGui::Render();

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),a_pCmdList);
}

void ImGuiContex::AddLog(const char* a_fmt, ...)
{
	if (!m_upLog) return;

	va_list _args;
	va_start(_args,a_fmt);
	m_upLog->AddLog(a_fmt);
	va_end(_args);
}

void ImGuiContex::AddLogMatrix(const std::string& a_name, const DirectX::XMFLOAT4X4& a_mat)
{
	AddLog("MatrixName : %s\n", a_name.c_str());

	for (size_t _row = 0; _row < 4; ++_row)
	{
		for (size_t _col = 0; _col < 4; ++_col)
		{
			AddLog("%f ", a_mat.m[_row][_col]);
		}
		AddLog("\n");
	}
}

void ImGuiContex::StartWatch(const std::string& a_name)
{
	auto _it = m_upWatchMap.find(a_name);
	if (_it != m_upWatchMap.end())
	{
		_it->second->Start();
	}
	else
	{
		m_upWatchMap[a_name] = std::make_unique<Watch>();
		m_upWatchMap[a_name]->Start();
	}
}

void ImGuiContex::EndWatch(const std::string& a_name)
{
	auto _it = m_upWatchMap.find(a_name);
	if (_it != m_upWatchMap.end())
	{
		_it->second->End();
		return;
	}
	assert(0 && "登録されていない計測です");
}

ImGuiContex::ImGuiContex()
{
}

ImGuiContex::~ImGuiContex()
{
}

void ImGuiContex::Release()
{
	m_upLog = nullptr;

	// メモリの解放
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
