#pragma once

class BaseScene;
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

enum class SceneChangeType
{
	Puch,		// 重ねる
	Pop,		// 一つ消去
	Replace,	// 切り替え
	Clear		// 全消去
};

struct SceneChangeCmd
{
	SceneType type = SceneType::None;
	SceneChangeType changeType = SceneChangeType::Replace;
};

class SceneManager
{
public:

	//------------------------------------------------------------------------------------------
	// メイン処理
	//------------------------------------------------------------------------------------------
	bool Init();				// 初期化
	void Release();				// 解放
	void Update(float a_dt);	// 更新
	void Draw(Engine::Graphics::RenderContext* a_pRCT);				// 描画

	//------------------------------------------------------------------------------------------
	// シーンの切り替え
	//------------------------------------------------------------------------------------------
	
	/// <summary>
	/// シーンの切り替え命令
	/// </summary>
	/// <param name="a_nextScene">切り替え先のシーンタイプ</param>
	/// <param name="a_changeType">切り替え方法</param>
	void SetNextScene(const SceneType& a_nextScene,const SceneChangeType& a_changeType);

	//------------------------------------------------------------------------------------------
	// 取得
	//------------------------------------------------------------------------------------------

	/// <summary>
	/// 現在のシーンのワールドを参照
	/// </summary>
	Engine::ECS::World* RefWorld();

private:

	//------------------------------------------------------------------------------------------
	// シーン
	//------------------------------------------------------------------------------------------
	void ChangeScenen();								// フレームの初めにシーンの切り替えを実行する
	void ReplaceScene(const SceneType& a_sceneType);	// シーンの切り替え
	void PushScene(const SceneType& a_sceneType);		// シーンを重ねる
	void PopScene();									// 最前面のシーンを消去

	template<typename Scene>
	void RegisterScene(const SceneType& a_sceneType);	// シーンの登録

private:

	// シーンスタック
	std::vector<std::unique_ptr<BaseScene>> m_upBaseSceneVec;
	std::unordered_map<SceneType, std::function<std::unique_ptr<BaseScene>()>> m_sceneCreateFuncMap;
	bool m_isOneUpdate = false;

	// シーン切り替え命令スタック
	std::queue<SceneChangeCmd> m_sceneChangeCmd = {};

	
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

template<typename Scene>
inline void SceneManager::RegisterScene(const SceneType& a_sceneType)
{
	// ベースシーンへ変換
	static_assert(std::is_base_of<BaseScene, Scene>::value, "ベースシーンへの変換に失敗しました");
	// 登録
	m_sceneCreateFuncMap[a_sceneType] = []() {return std::make_unique<Scene>(); };
}
