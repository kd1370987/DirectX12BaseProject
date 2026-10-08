#pragma once

#include "../SceneAmbient/SceneAmbient.h"
#include "../SceneConfig/SceneConfig.h"

namespace Engine
{
	namespace ECS
	{
		class World;
	}

	namespace GameObject
	{
		class GameObjectManager;
	}

	namespace Resource
	{
		class ResourceManager;
	}
}
namespace Engine::Scene
{
	//======================================================================================
	// シーン用のワールドを1つ作る
	//
	// ・ECSワールドの生成と初期化
	// ・アプリ寿命のサービス(ResourceManager / InputManager など)の差し込み
	// ・コンポーネントとシステムの登録(SceneManager のワールド初期化コールバック)
	// までを済ませたものを返す。
	//
	// 通常のシーン(BaseScene::Enter)だけでなく、エディターのプレビュー用シーン
	// (EffectEditor)も同じ構成のワールドを要るのでここに切り出してある。
	// 「ゲームのシーンと同じ環境で確認できる」ことが目的なので、
	// 片方だけ登録漏れが起きない形にしておきたい。
	//
	// a_isPreview : プレビュー用なら true。構成は同じで、物理空間の確保量だけ小さくする
	//======================================================================================
	std::unique_ptr<Engine::ECS::World> CreateSceneWorld(bool a_isPreview = false);

	class BaseScene
	{
	public:

		BaseScene();
		virtual ~BaseScene();

		/// <summary>
		/// 初期化
		/// </summary>
		void Enter();

		/// <summary>
		/// 解放 : エンティティを消すところまで
		/// </summary>
		/// <remarks>
		/// コンポーネントが借りているリソースは解放フックが返す。
		/// 共有しているもの(誰も持っていないリソースの破棄・当たり判定の空間)の
		/// 片付けは SceneManager::PopScene が持つ。
		/// </remarks>
		void Exit();

		/// <summary>
		/// 更新処理
		/// </summary>
		void Update(float a_dt);

		/// <summary>
		/// 描画処理
		/// </summary>
		void Draw();

		/// <summary>
		/// アーカイブ処理
		/// </summary>
		/// <param name="a_ar">保存・読み込み両方可</param>
		void  Archive(Persistence::Archive& a_ar);

		/// <summary>
		/// 先読み一覧(SceneConfig)のアセットの読み込みを要求する
		/// </summary>
		/// <remarks>
		/// 設定ファイルを読んだ後、シーンの中身を組み立てる(Archive)前に呼ぶ想定。
		/// 中身の組み立てが同じアセットを引きに来たときには、読み終わっているか読込中になっている。
		/// 重いもの(モデル・テクスチャ)はジョブへ流すので、組み立てと並んで読まれる。
		/// 消えたアセット・読めない種別は読み飛ばす。
		/// </remarks>
		void PreLoadAsset(Resource::ResourceManager& a_resourceManager);

		/// <summary>
		/// 現在のワールドを取得
		/// </summary>
		/// <returns></returns>
		Engine::ECS::World* RefWorld() { return m_upWorld.get(); }

		/// <summary>
		/// ECS外オブジェクトのマネージャーを取得(エディター用)。
		/// </summary>
		GameObject::GameObjectManager* RefGameObjectManager() { return m_upGameObjectManager.get(); }

		/// <summary>
		/// シーンの環境設定(環境光・平行光・影・フォグ・ボリュメトリックフォグ・空)
		/// </summary>
		/// <remarks>
		/// GraphicsEngine へ流し込むのは SceneManager(重なったシーンのどれを使うかを決めるため)。
		/// </remarks>
		SceneAmbient& RefAmbient() { return m_ambient; }
		const SceneAmbient& GetAmbient() const { return m_ambient; }

		/// <summary>
		/// シーンの設定(先読み一覧・計測フラグ)
		/// </summary>
		/// <remarks>
		/// シーンファイルとは別のファイルに保存される(理由は SceneConfig を参照)。
		/// </remarks>
		SceneConfig& RefConfig() { return m_config; }
		const SceneConfig& GetConfig() const { return m_config; }

		void SetGUID(const Core::GUID& a_guid) { m_guid = a_guid; }
		const Core::GUID& GetGUID() const { return m_guid; }


	private:
		// ゲームの本体 : ワールド空間上にあるものすべてはこちらで管理
		std::unique_ptr<Engine::ECS::World> m_upWorld = nullptr;

		// ECS側で扱いにくいものなどの管理
		std::unique_ptr<GameObject::GameObjectManager> m_upGameObjectManager = nullptr;

		// シーンの環境設定 : シーンと一緒に保存される
		SceneAmbient m_ambient = {};

		// 自身のデータの所在
		Core::GUID m_guid;

		// シーンの設定 : 開始時に読み込んでおきたいアセット(先読み一覧)と、その計測フラグ
		SceneConfig m_config = {};
	};
}