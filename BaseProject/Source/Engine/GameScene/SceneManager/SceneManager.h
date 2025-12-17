#pragma once

class ModelResource;

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

	std::shared_ptr<ModelResource> m_spModel;

	DirectX::XMFLOAT4X4 m_cameraMat;

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