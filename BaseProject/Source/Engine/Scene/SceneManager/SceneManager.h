#pragma once

namespace Engine
{
	namespace ECS
	{
		class World;
	}
	namespace Graphics
	{
		class RenderContext;
	}
}

namespace Engine::Scene
{
	class BaseScene;

	enum class SceneChangeType
	{
		Puch,		// 重ねる
		Pop,		// 一つ消去
		Replace,	// 切り替え
		Clear		// 全消去
	};



	class SceneManager
	{
	public:

		//------------------------------------------------------------------------------------------
		// メイン処理
		//------------------------------------------------------------------------------------------
		bool Init();										// 初期化
		void Release();										// 解放
		void Update(float a_dt);							// 更新
		void Draw();										// 描画

		// コールバック処理
		void SetWorldInitCallback(std::function<void(Engine::ECS::World* a_pWorld)> a_callback);	// セット
		void InvokeWorldInitCallback(Engine::ECS::World* a_pWorld);									// 呼び出し

		//------------------------------------------------------------------------------------------
		// シーンの切り替え
		//------------------------------------------------------------------------------------------

		/// <summary>
		/// シーンの切り替え命令
		/// </summary>
		/// <param name="a_nextScene">切り替え先のシーンタイプ</param>
		/// <param name="a_changeType">切り替え方法</param>
		void SetNextScene(const Engine::GUID& a_guid, const SceneChangeType& a_changeType);

		//------------------------------------------------------------------------------------------
		// 取得
		//------------------------------------------------------------------------------------------

		/// <summary>
		/// 現在のシーンのワールドを参照
		/// </summary>
		Engine::ECS::World* RefWorld();

		/// <summary>
		/// 現在の一番上のシーンを取得
		/// </summary>
		/// <returns>ベースシーンポインタ</returns>
		BaseScene* GetCurrentTopScene();

	private:

		//------------------------------------------------------------------------------------------
		// シーン
		//------------------------------------------------------------------------------------------
		void ChangeScenen();								// フレームの初めにシーンの切り替えを実行する
		void ReplaceScene(const Engine::GUID& a_guid);		// シーンの切り替え
		void PushScene(const Engine::GUID& a_guid);			// シーンを重ねる
		void PopScene();									// 最前面のシーンを消去

	private:

		struct SceneChangeCmd
		{
			Engine::GUID sceneGUID = Engine::DefaultGUID;
			SceneChangeType changeType = SceneChangeType::Replace;
		};

		// シーンスタック
		std::vector<std::unique_ptr<BaseScene>> m_upBaseSceneVec;
		bool m_isOneUpdate = false;

		// シーン切り替え命令スタック
		std::queue<SceneChangeCmd> m_sceneChangeCmd = {};

		// ワールドを設定するためのコールバック関数
		std::function<void(Engine::ECS::World* a_pWorld)> m_worldInitCallback = nullptr;

	private:
		// シングルトン化
		SceneManager();
		~SceneManager();

	public:
		// インスタンス取得
		static SceneManager& Instance()
		{
			static SceneManager _instance;
			return _instance;
		}
	};
}
