#include "App.h"

#include "../Core/RenderingEngine.h"
#include "../SceneManager/SceneManager.h"

HINSTANCE g_hInst;
HWND g_hWnd = NULL;

//==================================================================================
// 
// メッセージの取得
//
//==================================================================================
LRESULT CALLBACK WndProc(HWND a_hWnd, UINT a_msg, WPARAM a_wp, LPARAM a_lp)
{
	switch (a_msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		break;
	}

	return DefWindowProc(a_hWnd, a_msg, a_wp, a_lp);
}

//==================================================================================
// 
// ウィンドウの生成
// 
//==================================================================================
void InitWindow(const TCHAR* a_appName)
{
	// 実行ファイルのHINSTCEを返す
	g_hInst = GetModuleHandle(nullptr);
	if (g_hInst == nullptr)
	{
		// 取得に失敗したらガード
		return;
	}

	// ウィンドウの設定
	WNDCLASSEX _wc = {};										// 生成・初期化
	_wc.cbSize = sizeof(WNDCLASSEX);							// 構造体のサイズ
	_wc.style = CS_HREDRAW | CS_VREDRAW;						// 描画スタイル
	_wc.lpfnWndProc = WndProc;									// ウィンドウに送られたメッセージを確保
	_wc.hIcon = LoadIcon(g_hInst, IDI_APPLICATION);				// ウィンドウのアイコン（Alt + Tabとか）
	_wc.hCursor = LoadCursor(g_hInst, IDC_ARROW);				// ウィンドウ内のでふぉるのとのカーソル指定
	_wc.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);		// ウィンドウの背景塗りつぶしを指定
	_wc.lpszMenuName = nullptr;									// ウィンドウクラスのメニューリソースID
	_wc.lpszClassName = a_appName;								// ウィンドウクラス名（ユニークにする必要）
	_wc.hIconSm = LoadIcon(g_hInst, IDI_APPLICATION);			// タスクバーとかのアイコン

	// ウィンドウクラスの登録
	RegisterClassEx(&_wc);										// 同じクラス名は使わないこと

	// ウィンドウサイズの設定（クライアント領域の設定（描画できる中身の部分））
	RECT _rect = {};
	_rect.right = static_cast<LONG>(WINDOW_WIDTH);
	_rect.bottom = static_cast<LONG>(WINDOW_HEIGHT);

	// ウィンドウサイズを調整（バーや枠線分を自動で計算してくれる）
	auto _style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&_rect, _style, FALSE);

	// ウィンドウの生成
	g_hWnd = CreateWindowEx(
		0,								// 拡張スタイル
		a_appName,						// ウィンドウクラス名（登録されたものと同じもの）
		a_appName,						// タイトルバーの表示名
		_style,							// ウィンドウのスタイル
		CW_USEDEFAULT,					// Xの位置をOSに任せる
		CW_USEDEFAULT,					// Yの位置をOSに任せる
		_rect.right - _rect.left,		// 幅
		_rect.bottom - _rect.top,		// 高さ
		nullptr,						// トップレベルのウィンドウのため親ハンドルはない
		nullptr,						// メニューハンドルはない
		g_hInst,						// RegisterClassExと同じものを渡す
		nullptr							// 作成パラメタ。追加情報を渡すことができる
	);

	// ウィンドウを表示
	ShowWindow(g_hWnd, SW_SHOWNORMAL);

	// ウィンドウにフォーカスする
	SetFocus(g_hWnd);					// キー入力を受け取れるようになる
}

//==================================================================================
// 
// メインループ
// 
//==================================================================================
void MainLoop()
{
	MSG _msg = {};
	while (WM_QUIT != _msg.message)
	{
		if (PeekMessage(&_msg, nullptr, 0, 0, PM_REMOVE == TRUE))
		{
			TranslateMessage(&_msg);
			DispatchMessage(&_msg);
		}
		else
		{
			
			SceneManager::Instance().Update();					// 更新

			RenderingEngine::Instance().BeginRender();			// 描画開始
			{
				SceneManager::Instance().Draw();				// 描画
			}
			RenderingEngine::Instance().EndRender();			// 描画終了
		}
	}
}

//==================================================================================
// 
// 初回呼び出し
// 
//==================================================================================
void StartApp(const TCHAR* a_appName)
{
	// ウィンドウ生成
	InitWindow(a_appName);

	// 描画エンジンの初期化
	if (!RenderingEngine::Instance().Init(g_hWnd, WINDOW_WIDTH, WINDOW_HEIGHT))
	{
		return;
	}

	// シーンの初期化
	if (!SceneManager::Instance().Init())
	{
		return;
	}

	// メイン処理ループ
	MainLoop();
}
