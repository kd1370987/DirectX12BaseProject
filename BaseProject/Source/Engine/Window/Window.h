#pragma once

class Window
{
public:

	/// <summary>
	/// ウィンドウの作成
	/// </summary>
	/// <param name="a_clientWidth">横幅</param> 
	/// <param name="a_clientHeight">縦幅</param> 
	/// <param name="a_titleName">タイトルバーの名前</param> 
	/// <param name="a_windowClassName">クラス名</param> 
	/// <returns>初期化に成功したらtrue</returns>
	bool Create(
		UINT a_clientWidth,
		UINT a_clientHeight,
		const std::wstring& a_titleName,
		const std::wstring& a_windowClassName
	);

	/// <summary>
	/// ウィンドウメッセージ処理
	/// </summary>
	/// <returns>終了メッセージが来るとfalseを返す</returns>
	bool ProcessMessage();

	/// <summary>
	/// ウィンドウハンドルを参照
	/// </summary>
	const HWND& GetWindowHandle() { return m_hWnd; }

private:

	HWND		m_hWnd;			// ウィンドウハンドル
	HINSTANCE	m_hInst;		// ウィンドウインスタンス

	// クライアント領域サイズ
	UINT m_clientWidth = 0;
	UINT m_clientHeight = 0;
};