#include "NativeWindow.h"



extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//==================================================================================
// 
// メッセージの取得
//
//==================================================================================
LRESULT CALLBACK WndProc(HWND a_hWnd, UINT a_message, WPARAM a_wParam, LPARAM a_lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(a_hWnd, a_message, a_wParam, a_lParam))
		return true;

	// ウィンドウズからのメッセージを処理
	switch (a_message)
	{
	case WM_DESTROY:				// OSに対して終了を伝える
		PostQuitMessage(0);
		break;
	default:
		break;
	}

	// メッセージの基本的な処理
	return DefWindowProc(a_hWnd, a_message, a_wParam, a_lParam);
}

//==================================================================================
// 
// ウィンドウの作成
//
//==================================================================================
namespace Engine::Window
{
	bool NativeWindow::Create(const  WindowDesc& a_desc)
	{
		// 実行ファイルのインスタンスハンドル取得
		m_hInst = GetModuleHandle(nullptr);
		if (!m_hInst)
		{
			assert(0 && "インスタンスハンドルの取得に失敗");
			return false;
		}

		// ウィンドウの仕様書作成
		WNDCLASSEX _wc = {};
		_wc.cbSize = sizeof(WNDCLASSEX);									// 構造体サイズ
		_wc.style = CS_OWNDC;												// 描画スタイル
		_wc.lpfnWndProc = (WNDPROC)WndProc;									// ウィンドウ関数
		_wc.hIcon = LoadIcon(m_hInst, IDI_APPLICATION);						// ウィンドウのアイコン（Alt+Tabなど）
		_wc.hCursor = LoadCursor(m_hInst, IDC_ARROW);						// ウィンドウ内のデフォルトカーソル
		_wc.hInstance = m_hInst;											// インスタンスハンドル
		_wc.hbrBackground = nullptr;										// 初期の背景塗りつぶし色
		_wc.lpszMenuName = nullptr;											// ウィンドウクラスのメニューリソースID
		_wc.lpszClassName = a_desc.className.c_str();						// ウィンドウのクラス名（ユニーク性必須）
		_wc.hIconSm = LoadIcon(m_hInst, IDI_APPLICATION);					// タスクバーとかのアイコン
		// ウィンドウクラスの登録（同じクラス名は使わないこと）
		if (!RegisterClassEx(&_wc))
		{
			assert(0 && "ウィンドウクラスの登録失敗");
			return false;
		}

		// モードに応じたスタイルの設定
		DWORD _style = 0;
		DWORD _exStyle = 0;
		int _x = CW_USEDEFAULT;
		int _y = CW_USEDEFAULT;
		UINT _width = a_desc.width;
		UINT _height = a_desc.height;

		switch (a_desc.windowMode)
		{
			case EWindowMode::Windowed:
				_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
				break;
			case EWindowMode::FullScreen:
			case EWindowMode::Borederless:
				// 枠なしスタイル
				_style = WS_POPUP | WS_VISIBLE;
				_x = 0;
				_y = 0;
				// モニターの解像度を取得してウィンドウサイズにする
				_width = GetSystemMetrics(SM_CXSCREEN);
				_height = GetSystemMetrics(SM_CYSCREEN);
				break;
		}

		// ウィンドウサイズの調整（クライアント領域の設定 = 描画できる中身の部分）
		RECT _rect = { 0,0,static_cast<LONG>(_width),static_cast<LONG>(_height) };
		if (a_desc.windowMode == EWindowMode::Windowed)
		{
			// ウィンドウモードの時のみ枠の計算をする（フルスクリーンとボーダレスは枠なしスタイルなので必要ない）
			AdjustWindowRect(&_rect, _style, FALSE);
		}

		// ウィンドウの生成
		m_hWnd = CreateWindowEx(
			_exStyle,					// 拡張スタイル
			a_desc.className.c_str(),	// ウィンドウのクラス名（登録したのと同じもの）
			a_desc.titleName.c_str(),		// タイトルバーの名前
			_style,						// ウィンドウのスタイル
			_x,							// ウィンドウの X座標
			_y,							// ウィンドウの Y座標
			_rect.right - _rect.left,	// 幅
			_rect.bottom - _rect.top,	// 高さ
			nullptr,					// 親ウィンドウハンドル（トップレベルのため無し）
			nullptr,					// メニューハンドル（無し）
			m_hInst,					// 登録したときと同じハンドルを渡す
			nullptr						// 作成パラメタ　追加オプション
		);
		if (!m_hWnd) {
			DWORD err = GetLastError();
			assert(0 && "CreateWindowEx failed");
			return false;
		}

		// ウィンドウを表示
		ShowWindow(m_hWnd, SW_SHOWNORMAL);

		// ウィンドウの更新
		UpdateWindow(m_hWnd);

		// ウィンドウにフォーカスする
		SetFocus(m_hWnd);					// キー入力受付開始命令も含む

		// メンバ変数の保存
		m_className = a_desc.className;
		m_clientWidth = a_desc.width;
		m_clientHeight = a_desc.height;
		m_windowMode = a_desc.windowMode;

		return true;
	}

	void NativeWindow::Release()
	{
		// ウィンドウの解放
		UnregisterClass(m_className.c_str(), m_hInst);
	}

	bool NativeWindow::ProcessMessage()
	{
		// アプリごとのメッセージキューにアクセスしてメッセージを処理
		MSG _msg = {};
		while (PeekMessage(&_msg, nullptr, 0, 0, PM_REMOVE))
		{
			// 終了メッセージが来た
			if (_msg.message == WM_QUIT)
			{
				return false;
			}

			// メッセージ処理
			TranslateMessage(&_msg);
			DispatchMessage(&_msg);
		}

		return true;
	}

	void NativeWindow::ChangeTitle(const std::string& a_title)
	{
		SetWindowTextA(m_hWnd, a_title.c_str());
	}
	double NativeWindow::GetMemoryUsage()
	{
		PROCESS_MEMORY_COUNTERS_EX _pmc;
		if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&_pmc, sizeof(_pmc)))
		{
			// WorkingSetSizeがタスクマネージャーのメモリに一番近い数値
			// 物理メモリ使用量
			SIZE_T _physMemUsed = _pmc.WorkingSetSize;

			return static_cast<double>(_physMemUsed);
		}
	}
}