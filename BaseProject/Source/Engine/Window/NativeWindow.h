#pragma once

namespace Engine::Window
{
	enum class EWindowMode
	{
		Windowed,		// ウィンドウモード
		FullScreen,		// フルスクリーンモード
		Borederless,	// ボーダレスモード
	};

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

		// アクセサ
		const HWND& GetWindowHandle() const { return m_hWnd; }
		const UINT& GetClientWidth() const { return m_clientWidth; }
		const UINT& GetClientHeight() const { return m_clientHeight; }

		// メモリ使用率取得
		// バイト単位での取得
		double GetMemoryUsage();

	private:
		// Windows用
		HWND		m_hWnd;			// ウィンドウハンドル
		HINSTANCE	m_hInst;		// ウィンドウインスタンス
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
	};
}