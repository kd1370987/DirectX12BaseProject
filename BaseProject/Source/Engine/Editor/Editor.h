#pragma once

class Log;
class Watch;

class RenderGraphView;
class ECSView;

class ComponentEdit;

class AssetResourceView;

namespace Engine::Editor
{
	// 前方宣言
	class ImGuiContext;

	//=======================================================================
	// 
	// メインエディタクラス
	// 
	//=======================================================================
	class MainEditor
	{
	public:

		// 初期化
		bool Init(HWND a_hwnd);

		// 解放
		void Release();

		// 描画
		void Draw(ID3D12GraphicsCommandList* a_pCmdList);

		// コンポーネントエディットの取得
		std::shared_ptr<ComponentEdit> GetCompEdit();

	//=======================================================================
	// ログ関連
	//=======================================================================
	public:
		// ログの追加
		// 文字列と可変引数でログを追加する関数
		void AddLog(const char* a_fmt, ...);
		// 行列をログに追加する関数
		void AddLogMatrix(const std::string& a_name, const DirectX::XMFLOAT4X4& a_mat);

	//=======================================================================
	// 計測関連
	//=======================================================================
	public:
		// 計測開始
		void StartWatch(const std::string& a_name);
		// 計測終了
		void EndWatch(const std::string& a_name);

	private:
		
		// ImGuiコンテキスト
		std::unique_ptr<ImGuiContext> m_upImGuiContext = nullptr;

		// ログ
		std::unique_ptr<Log> m_upLog = nullptr;

		// 計測
		std::unordered_map<std::string, std::unique_ptr<Watch>> m_upWatchMap = {};

		// レンダーグラフビュー
		std::unique_ptr<RenderGraphView> m_upRGView = nullptr;
		// ECS
		std::unique_ptr<ECSView> m_upECSView = nullptr;

		// アセットビュー
		std::unique_ptr<AssetResourceView> m_upAssetResourceView = nullptr;

		bool m_isInit = false;

	private:
		MainEditor();
		~MainEditor();
	public:
		// コピー禁止
		MainEditor(const MainEditor&) = delete;
		MainEditor& operator=(const MainEditor&) = delete;
		// ムーブ禁止
		MainEditor(MainEditor&&) = delete;
		MainEditor& operator=(MainEditor&&) = delete;

		// シングルトンインスタンス取得
		static MainEditor& Instance()
		{
			static MainEditor _instance;
			return _instance;
		}
	};
}