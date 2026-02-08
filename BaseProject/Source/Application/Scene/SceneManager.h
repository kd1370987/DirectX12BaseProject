#pragma once

class BaseScene;

class SceneManager
{
public:

	/// <summary>
	/// シーンの生成、初期化
	/// </summary>
	/// <returns>成功 = true</returns>
	bool Init();

	/// <summary>
	/// シーンが持つものを解放1
	/// </summary>
	void Release();

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update(float a_dt);

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// シーンの追加
	/// </summary>
	/// <param name="a_sceneType">追加したいタイプ</param>
	/// ポーズ、オプションなど
	void PushScene(const SceneType& a_sceneType);	

	/// <summary>
	/// 最前面のシーンを消去
	/// </summary>
	void PopScene();

	/// <summary>
	/// シーンの切り替え
	/// </summary>
	/// <param name="a_sceneType">切り替え後のタイプ</param>
	void ReplaceScene(const SceneType& a_sceneType);

private:

	/// <summary>
	/// シーンの登録
	/// </summary>
	/// <typeparam name="Scene">シーンタイプ</typeparam>
	/// <param name="a_sceneType">登録するシーンクラス</param>
	template<typename Scene>
	void RegisterScene(const SceneType& a_sceneType)
	{
		// ベースシーンへ変換
		static_assert(std::is_base_of<BaseScene, Scene>::value, "ベースシーンへの変換に失敗しました");
		// 登録
		m_sceneCreateFuncMap[a_sceneType] = []() {return std::make_unique<Scene>(); };
	}

private:

	// シーンスタック
	std::vector<std::unique_ptr<BaseScene>> m_upBaseSceneVec;
	std::unordered_map<SceneType, std::function<std::unique_ptr<BaseScene>()>> m_sceneCreateFuncMap;
	bool m_isOneUpdate = false;

	
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