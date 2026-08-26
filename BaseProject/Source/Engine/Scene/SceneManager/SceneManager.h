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
	namespace GameObject
	{
		class GameObjectManager;
	}
}

namespace Engine::Scene
{
	class BaseScene;

	enum class SceneChangeType
	{
		Push,		// 重ねる
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
		void Release();										// 解放
		void Update(float a_dt);							// 更新
		void Draw();										// 描画

		//------------------------------------------------------------------------------------------
		// ワールドの作り手
		//
		// シーンが持つワールドの「種類」を決めるのは上位層(App::ECS::World)。
		// エンジンは基盤の Engine::ECS::World としてしか触らないので、
		// 実体を作るところだけ差し込んでもらう。
		//
		// 差し込み忘れるとシーンにワールドが無い状態になるため、CreateWorld はログを出す。
		//------------------------------------------------------------------------------------------
		using WorldFactory = std::function<std::unique_ptr<Engine::ECS::World>()>;

		void SetWorldFactory(WorldFactory a_factory);			// セット
		std::unique_ptr<Engine::ECS::World> CreateWorld();		// 呼び出し

		//------------------------------------------------------------------------------------------
		// シーンの新規作成
		//------------------------------------------------------------------------------------------

		/// <summary>
		/// 空のシーンをアセットとして作る
		/// </summary>
		/// <param name="a_path">Asset/Scenes/ からの相対フォルダ。空でもよい</param>
		/// <param name="a_name">シーン名。フォルダ名にもファイル名にも使う</param>
		/// <returns>作ったシーンのGUID。失敗したら無効なGUID</returns>
		/// <remarks>
		/// 中身は空のまま書き出すだけで、今開いているシーンには触らない。
		/// 開きたい場合は返ったGUIDで SetNextScene を呼ぶこと。
		///
		/// 書き出しは実物の BaseScene を1つ作って Archive へ流す。
		/// 「空のシーンファイルの中身」を手で組み立てると、
		/// BaseScene::Archive に項目が増えたときに古い形のまま作り続けてしまう
		/// (バイナリ(.obscene)は並び順で読むので、そのまま壊れる)
		/// </remarks>
		Engine::GUID CreateEmptyScene(const std::string& a_path, const std::string& a_name);

		//------------------------------------------------------------------------------------------
		// シーンの切り替え
		//------------------------------------------------------------------------------------------

		/// <summary>
		/// シーンの切り替え命令
		/// </summary>
		/// <param name="a_nextScene">切り替え先のシーンタイプ</param>
		/// <param name="a_changeType">切り替え方法</param>
		void SetNextScene(const Engine::GUID& a_guid, const SceneChangeType& a_changeType);

		/// <summary>
		/// 更新するのを一番上のシーンだけにするか
		/// </summary>
		/// <remarks>
		/// 既定は true。ポーズ画面のように重ねたシーンを出している間、
		/// 後ろのシーンは描画だけ続けて更新は止まる。
		/// 後ろも動かしたい重ね方をするときだけ false にすること。
		/// </remarks>
		void SetUpdateTopSceneOnly(bool a_isTopOnly) { m_isUpdateTopSceneOnly = a_isTopOnly; }
		bool IsUpdateTopSceneOnly() const { return m_isUpdateTopSceneOnly; }

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

		/// <summary>
		/// 現在のシーンのECS外オブジェクトマネージャーを参照(エディター用)
		/// </summary>
		GameObject::GameObjectManager* RefGameObjectManager();

	private:

		//------------------------------------------------------------------------------------------
		// シーン
		//------------------------------------------------------------------------------------------
		void ChangeScenen();								// フレームの初めにシーンの切り替えを実行する
		void ReplaceScene(const Engine::GUID& a_guid);		// シーンの切り替え
		bool PushScene(const Engine::GUID& a_guid);			// シーンを重ねる(読み込めたら true)
		void PopScene();									// 最前面のシーンを消去

	private:

		struct SceneChangeCmd
		{
			Engine::GUID sceneGUID = Engine::DefaultGUID;
			SceneChangeType changeType = SceneChangeType::Replace;
		};

		// シーンスタック
		std::vector<std::unique_ptr<BaseScene>> m_upBaseSceneVec;

		// 更新するのは一番上のシーンだけか(重ねたシーンの後ろを止めるための既定)
		bool m_isUpdateTopSceneOnly = true;

		// シーン切り替え命令スタック
		std::queue<SceneChangeCmd> m_sceneChangeCmd = {};

		// ワールドの実体を作る関数(上位層が差し込む)
		WorldFactory m_worldFactory = nullptr;

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
