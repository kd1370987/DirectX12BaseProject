#pragma once

class RenderingEngine;
class DescriptorHeapManager;

class Log;
class Watch;

class RenderGraphView;

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
	/// <summary>
	/// ログの追加 : 行列用
	/// </summary>
	void AddLogMatrix(const std::string& a_name, const DirectX::XMFLOAT4X4& a_mat);


	/// <summary>
	/// 時間を測るときに使用する関数
	/// </summary>
	/// <param name="a_name">ウィンドウネーム</param>
	void StartWatch(const std::string& a_name);
	/// <summary>
	/// 時間を測るときに使用する関数
	/// </summary>
	/// <param name="a_name">ウィンドウネーム</param>
	void EndWatch(const std::string& a_name);

private:

	// ログ
	std::unique_ptr<Log> m_upLog = nullptr;

	// 計測
	std::unordered_map<std::string, std::unique_ptr<Watch>> m_upWatchMap = {};

	// レンダーグラフビュー
	std::unique_ptr<RenderGraphView> m_upRGView = nullptr;

	bool m_isInit = false;
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