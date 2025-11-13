#include "Window.h"

//==================================================================================
// 
// メッセージの取得
//
//==================================================================================
LRESULT CALLBACK WndProc(HWND a_hWnd, UINT a_message, WPARAM a_wParam, LPARAM a_lParam)
{
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

bool Window::Create(
	UINT a_clientWidth, 
	UINT a_clientHeight, 
	const std::wstring& a_titleName, 
	const std::wstring& a_windowClassName
)
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
	_wc.cbSize = sizeof(WNDCLASSEX);						// 構造体サイズ
	_wc.style = CS_HREDRAW | CS_VREDRAW;					// 描画スタイル
	_wc.lpfnWndProc = (WNDPROC)WndProc;						// ウィンドウ関数
	_wc.hIcon = LoadIcon(m_hInst, IDI_APPLICATION);			// ウィンドウのアイコン（Alt+Tabなど）
	_wc.hCursor = LoadCursor(m_hInst, IDC_ARROW);			// ウィンドウ内のデフォルトカーソル
	_wc.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);	// 初期の背景塗りつぶし色
	_wc.lpszMenuName = nullptr;								// ウィンドウクラスのメニューリソースID
	_wc.lpszClassName = a_windowClassName.c_str();			// ウィンドウのクラス名（ユニーク性必須）
	_wc.hIconSm = LoadIcon(m_hInst, IDI_APPLICATION);		// タスクバーとかのアイコン

	// ウィンドウクラスの登録（同じクラス名は使わないこと）
	if (!RegisterClassEx(&_wc))
	{
		assert(0 && "ウィンドウクラスの登録失敗");
		return false;
	}

	// ウィンドウサイズの設定（クライアント領域の設定 = 描画できる中身の部分）
	RECT _rect = {};
	_rect.right = static_cast<LONG>(a_clientWidth);
	_rect.bottom = static_cast<LONG>(a_clientHeight);

	// ウィンドウサイズの調整（バーや枠線分を自動計算してくれるもの）
	auto _style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&_rect, _style, FALSE);

	// ウィンドウの生成
	m_hWnd = CreateWindowEx(
		0,							// 拡張スタイル
		a_windowClassName.c_str(),	// ウィンドウのクラス名（登録したのと同じもの）
		a_titleName.c_str(),		// タイトルバーの名前
		_style,						// ウィンドウのスタイル
		CW_USEDEFAULT,				// ウィンドウの X座標（OSに任せる）
		CW_USEDEFAULT,				// ウィンドウの Y座標（OSに任せる）
		_rect.right - _rect.left,	// 幅
		_rect.bottom - _rect.top,	// 高さ
		nullptr,					// 親ウィンドウハンドル（トップレベルのため無し）
		nullptr,					// メニューハンドル（無し）
		m_hInst,					// 登録したときと同じハンドルを渡す
		nullptr						// 作成パラメタ　追加オプション
	);

	// ウィンドウを表示
	ShowWindow(m_hWnd, SW_SHOWNORMAL);

	// ウィンドウの更新
	UpdateWindow(m_hWnd);

	// ウィンドウにフォーカスする
	SetFocus(m_hWnd);					// キー入力受付開始命令も含む
}

bool Window::ProcessMessage()
{
	// アプリごとのメッセージキューにアクセスしてメッセージを処理
	MSG _msg;
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
