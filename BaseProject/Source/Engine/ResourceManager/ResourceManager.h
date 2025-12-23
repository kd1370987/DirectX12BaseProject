#pragma once

class Texture;

class ResourceManager
{
public:
	
	void Init();	// 初期化

	std::weak_ptr<Texture> GetTexture(const std::string& a_key);

private:
	
	Storage<std::string, Texture>		m_textureStorage;	// テクスチャストレージ

private:

	ResourceManager() = default;
	~ResourceManager() = default;

public:

	static ResourceManager& Instance()
	{
		static ResourceManager _instance;
		return _instance;
	}
};