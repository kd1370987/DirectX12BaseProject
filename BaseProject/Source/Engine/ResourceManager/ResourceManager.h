#pragma once

class Texture;
class ModelResource;

class ResourceManager
{
public:
	
	void Init();	// 初期化

	/// <summary>
	/// テクスチャの取得
	/// </summary>
	/// <param name="a_key">ファイルパス</param>
	/// <returns>テクスチャのウィークポインタ</returns>
	std::weak_ptr<Texture> GetTexture(const std::string& a_key);

	/// <summary>
	/// モデル取得
	/// </summary>
	/// <param name="a_key">ファイルパス</param>
	/// <returns>モデルのウィークポインタ</returns>
	std::weak_ptr<ModelResource> GetModel(const std::string& a_key);

private:
	
	Storage<std::string, Texture>		m_textureStorage;	// テクスチャストレージ

	Storage<std::string, ModelResource> m_modelStorage;		// モデルストレージ

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