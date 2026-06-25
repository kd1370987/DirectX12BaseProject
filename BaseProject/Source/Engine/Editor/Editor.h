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
	class RenderGraphResourceView;

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
		// 計測開始
		void StartWatch(const std::string& a_name);
		// 計測終了
		void EndWatch(const std::string& a_name);

		//=======================================================================
		// デバッグ描画用
		//=======================================================================
		// ライン描画
		void DrawLine(
			const DirectX::SimpleMath::Vector3& a_startPos,
			const DirectX::SimpleMath::Vector3& a_endPos,
			const DirectX::SimpleMath::Color& a_color = Color::WHITE
		);

		void DrawBox(const DirectX::SimpleMath::Matrix& a_worldMat, const DirectX::SimpleMath::Color& a_color = Color::WHITE);
		void DrawBox(const DirectX::BoundingBox& a_aabb, const DirectX::SimpleMath::Color& a_color = Color::WHITE);
		void DrawBox(const DirectX::BoundingOrientedBox& a_obb, const DirectX::SimpleMath::Color& a_color = Color::WHITE);
		void DrawCapsule(const DirectX::SimpleMath::Matrix& a_worldMat, const DirectX::SimpleMath::Color& a_color = Color::WHITE);
		void DrawSphere(const DirectX::SimpleMath::Matrix& a_worldMat, const DirectX::SimpleMath::Color& a_color = Color::WHITE);
		void DrawSphere(const DirectX::BoundingSphere& a_sphere, const DirectX::SimpleMath::Color& a_color = Color::WHITE);

		// レイとヒット時に球体を出す
		void DrawRay(
			const DirectX::SimpleMath::Vector3& a_startPos,
			const DirectX::SimpleMath::Vector3& a_dir,
			float a_length,
			bool a_isHit,
			const DirectX::SimpleMath::Color& a_color = Color::WHITE
		);

		void ClearBuffer();
		const std::vector<Graphics::DebugLineData>& GetDebugLineDataVec() const;

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
		std::unique_ptr<RenderGraphResourceView> m_upRenderGraphResourceView = nullptr;

		// 計測機
		std::unique_ptr<WatchView> m_upWatchView = nullptr;

		// デバッグ描画用データ
		std::vector<Graphics::DebugLineData> m_debugLineDataVec = {};
		UINT m_debugLineDataCapacity = 10000;

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