#pragma once

namespace Engine::Window
{


	struct WindowDesc
	{
		UINT width = 0;						// ウィンドウの横幅
		UINT height = 0;					// ウィンドウの縦幅
		std::wstring titleName = L"Game";				// タイトルバーの名前
		std::wstring className = L"GameWindow";			// クラス名
		EWindowMode windowMode = EWindowMode::Windowed;	// ウィンドウモード
	};

	class NativeWindow
	{
	public:

		// ウィンドウの作成
		bool Create(const WindowDesc& a_desc);

		// 解放処理
		void Release();

		// メッセージ処理
		bool ProcessMessage();

		// タイトルの変更
		void ChangeTitle(const std::string& a_title);

		// ウィンドウモードの変更
		void ChangeWindowMode(EWindowMode a_nextWindowMode);

		/// <summary>
		/// HWNDから実際のクライアント領域サイズを取り直す
		/// ウィンドウプロシージャ(WM_SIZE)からも呼ばれる
		/// </summary>
		void RefreshClientSize();

		// アクセサ
		const HWND& GetWindowHandle() const { return m_hWnd; }
		const UINT& GetClientWidth() const { return m_clientWidth; }
		const UINT& GetClientHeight() const { return m_clientHeight; }

		// メモリ使用率取得
		// バイト単位での取得
		double GetMemoryUsage();

	private:

		/// <summary>
		/// クライアント領域が要求サイズになるようウィンドウ全体のサイズを補正する
		/// 枠の太さはDPIによって変わるため、作成後の実測値から差分を求めて合わせる。
		/// モニターの作業領域に収まらない場合は収まるところまで縮める
		/// </summary>
		void FitClientSize(UINT a_width, UINT a_height);

	private:
		// Windows用
		// ウィンドウプロシージャは CreateWindowEx の途中からも呼ばれるため、
		// ハンドルは必ずnullptrで初期化しておく
		HWND		m_hWnd = nullptr;	// ウィンドウハンドル
		HINSTANCE	m_hInst = nullptr;	// ウィンドウインスタンス
		std::wstring m_className;	// ウィンドウクラスネーム

		// ウィンドウモード時のサイズと位置を記憶しておく
		// フルスクリーンから戻した際に元のサイズと位置に戻すため
		RECT m_windowedRect = { 0,0,0,0 };

		// スタイル保持
		DWORD m_windowedStyle = WS_OVERLAPPEDWINDOW;

		// ウィンドウ設定
		UINT m_clientWidth = 0;
		UINT m_clientHeight = 0;

		// ウィンドウモード
		EWindowMode m_windowMode = EWindowMode::Windowed;

		// 現在のウィンドウ設定
		WINDOWPLACEMENT m_windowPlacement = {};
	};
}