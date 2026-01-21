#pragma once

class Texture;
class ModelResource;

using ModelID = uint32_t;
constexpr ModelID INVALID_MODEL_ID = static_cast<ModelID>(-1);

class GraphicResourceManager
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
	const ModelID& GetModel(const std::string& a_path);

	/// <summary>
	/// モデル取得
	/// </summary>
	/// <param name="a_modelID">登録ID</param>
	/// <returns>生ポインタ</returns>
	ModelResource* NGetModelResource(UINT a_modelID);

private:
	
	Storage<std::string, Texture>		m_textureStorage;	// テクスチャストレージ


	
	std::unordered_map<std::string, UINT> m_modelIDMap;
	Storage<ModelID, ModelResource> m_modelIDStorage; // モデルIDストレージ

	UINT m_count = 0;

private:

	GraphicResourceManager() = default;
	~GraphicResourceManager() = default;

public:

	static GraphicResourceManager& Instance()
	{
		static GraphicResourceManager _instance;
		return _instance;
	}
};