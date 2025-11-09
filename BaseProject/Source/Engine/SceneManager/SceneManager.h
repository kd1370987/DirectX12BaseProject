#pragma once

class SceneManager
{
public:

	bool Init();
	bool Release();

	void Update();

	void Draw();

private:
	// シーン
	
	float m_rotateY;

private:
	// シングルトン化
	SceneManager() {}
	~SceneManager() {}
public:
	// インスタンス取得
	static SceneManager& Instance()
	{
		static SceneManager _instance;
		return _instance;
	}
};