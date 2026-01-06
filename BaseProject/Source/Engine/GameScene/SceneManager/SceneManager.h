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

	std::shared_ptr<ModelResource> m_spModel[5];
	std::shared_ptr<ModelResource> m_spModel2;

	DirectX::XMFLOAT4X4 m_cameraMat;

	DirectX::XMFLOAT4X4 m_charaMat;
	DirectX::XMFLOAT4X4 m_charaMat2;

	
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