#pragma once
namespace Engine::Editor
{
	// 前方宣言
	class Log;
	class Watch;
	class ECSView;
	class ComponentEdit;
	class AssetResourceView;
	class ImGuiContext;
	class SceneView;
	class EditorCamera;
	class WatchView;

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
		void Draw(ID3D12GraphicsCommandList* a_pCmdList,UINT a_widht,UINT a_height);

		const EditorCamera* GetEditorCamera();

		//=======================================================================
		// ログ関連
		//=======================================================================
	public:
		// ログの追加
		// 文字列と可変引数でログを追加する関数
		void AddLog(const char* a_fmt, ...);
		void AddLogVector(const float* a_data,const size_t& a_size);
		// 行列をログに追加する関数
		void AddLogMatrix(const std::string& a_name, const DirectX::XMFLOAT4X4& a_mat);

		// 処理はおとさないが、不都合な処理が走った警告を出す用のログ
		void WarningLog(const char* a_fmt,...);

		// 処理を落とす際に呼び出す
		void ErrorLog(const char* a_fmt, ...);

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

		// ECS
		std::unique_ptr<ECSView> m_upECSView = nullptr;

		// シーンビュー
		std::unique_ptr<SceneView> m_upSceneView = nullptr;

		// アセットビュー
		std::unique_ptr<AssetResourceView> m_upAssetResourceView = nullptr;

		// 計測機
		std::unique_ptr<WatchView> m_upWatchView = nullptr;

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