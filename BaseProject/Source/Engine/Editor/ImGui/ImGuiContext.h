#pragma once

class RenderingEngine;
class DescriptorHeapManager;

class Log;
class Watch;

class ImGuiContex
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="a_hwnd">メインウィンドウハンドル</param>
	bool Init(HWND a_hwnd);

	/// <summary>
	/// 解放
	/// </summary>
	void Release();

	/// <summary>
	/// ImGui描画
	/// </summary>
	/// <param name="a_pCmdList">フレームで使用中のコマンドリス</param>
	void CallImGuiDrawData(ID3D12GraphicsCommandList* a_pCmdList);

	/// <summary>
	/// ログの追加
	/// </summary>
	/// <param name="a_fmt">引数あり文字列</param>
	void AddLog(const char* a_fmt, ...);

	void StartWatch(const std::string& a_name);
	void EndWatch(const std::string& a_name);
private:

	// ログ
	std::unique_ptr<Log> m_upLog = nullptr;

	// 計測
	std::unordered_map<std::string, std::unique_ptr<Watch>> m_upWatchMap = {};

private:

	ImGuiContex();
	~ImGuiContex();

public:

	static ImGuiContex& Instance()
	{
		static ImGuiContex _instance;
		return _instance;
	}


};