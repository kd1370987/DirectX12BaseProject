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
		struct DirectionalLight;
	}
	namespace GameObject
	{
		class GameObjectManager;
	}
	namespace Resource
	{
		class AssetDatabase;
		class ResourceManager;
	}
}

namespace Engine::Scene
{
	class BaseScene;

	enum class ESceneChangeType
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
		// 更新 : シーンの切り替え命令もここで消化する。
		// 行き先のファイルをGUIDから引き、シーンを外したときに使われなくなったリソースを捨てるので、
		// リソースマネージャーを受け取る(アセットデータベースもこの中)
		void Update(Resource::ResourceManager& a_resourceManager, float a_dt);
		void Draw();										// 描画

		//------------------------------------------------------------------------------------------
		// ワールドの作り手
		//
		// シーンが持つワールドの「種類」を決めるのは上位層(App::ECS::APPWorld)。
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
		Core::GUID CreateEmptyScene(Resource::AssetDatabase& a_assetDB, const std::string& a_path, const std::string& a_name);

		//------------------------------------------------------------------------------------------
		// シーンの切り替え
		//------------------------------------------------------------------------------------------

		/// <summary>
		/// シーンの切り替え命令
		/// </summary>
		/// <param name="a_nextScene">切り替え先のシーンタイプ</param>
		/// <param name="a_changeType">切り替え方法</param>
		void SetNextScene(const Core::GUID& a_guid, const ESceneChangeType& a_changeType);

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
		/// <remarks>
		/// 読み込み中のシーンがあるあいだは、そのシーンのワールドを返す。
		///
		/// シーンの読み込み(PushScene)は「ワールドを作る → 保存データを流し込む →
		/// スタックへ積む」の順なので、流し込んでいる最中はまだスタックに乗っていない。
		/// その間にワールドを要るもの(プレハブのように、コンポーネントのメタ情報が無いと
		/// 読めないリソース)を読むと、一つ前のシーンのワールドか nullptr が返ってしまい、
		/// 読めなかったことに気付かないまま空の実体がキャッシュに載る。
		/// </remarks>
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

		/// <summary>
		/// 環境設定を GraphicsEngine へ流し込んでいるシーン
		/// </summary>
		/// <remarks>
		/// 積んであるシーンのうち、環境設定を使う(SceneAmbient::IsEnabled)一番上のもの。
		/// 1つも無ければ nullptr(環境光・フォグ・空・平行光なしで描かれる)
		/// </remarks>
		BaseScene* GetAmbientSourceScene();

	private:

		//------------------------------------------------------------------------------------------
		// シーン
		//------------------------------------------------------------------------------------------
		void ChangeScenen(Resource::ResourceManager& a_resourceManager);								// フレームの初めにシーンの切り替えを実行する
		void ReplaceScene(Resource::ResourceManager& a_resourceManager, const Core::GUID& a_guid);	// シーンの切り替え
		bool PushScene(Resource::ResourceManager& a_resourceManager, const Core::GUID& a_guid);		// シーンを重ねる(読み込めたら true)
		void PopScene(Resource::ResourceManager& a_resourceManager);									// 最前面のシーンを消去

		//------------------------------------------------------------------------------------------
		// シーンの環境設定
		//------------------------------------------------------------------------------------------
		// 環境設定を使う一番上のシーンのものを GraphicsEngine へ流し込む(無ければ「無し」を流す)
		void ApplySceneAmbient();

	private:

		struct SceneChangeCmd
		{
			Core::GUID sceneGUID = Core::DEFAULT_GUID;
			ESceneChangeType changeType = ESceneChangeType::Replace;
		};

		// シーンスタック
		std::vector<std::unique_ptr<BaseScene>> m_upBaseSceneVec;

		// 今読み込んでいるシーン。スタックへ積むまでの間だけ入る(RefWorld がこれを優先する)
		BaseScene* m_pLoadingScene = nullptr;

		// 更新するのは一番上のシーンだけか(重ねたシーンの後ろを止めるための既定)
		bool m_isUpdateTopSceneOnly = true;

		// シーン切り替え命令スタック
		std::queue<SceneChangeCmd> m_sceneChangeCmd = {};

		// ワールドの実体を作る関数(上位層が差し込む)
		WorldFactory m_worldFactory = nullptr;

		// 環境設定の平行光が借りている LightManager の席。
		// シーンごとではなくここで1つだけ持つ : 重ねたシーンの数だけ太陽が並ばないように
		Handle<Graphics::DirectionalLight> m_ambientDLHandle = {};

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
