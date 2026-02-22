#pragma once

#include "ResourceSlotStorage/ResourceSlotStorage.h"

enum class TextureUse
{
	Albedo,
	MetallicRoughness,
	Emissive,
	Normal
};

class GraphicResourceManager
{
public:
	
	/// <summary>
	/// 初期化
	/// </summary>
	void Init();

	/// <summary>
	/// 解放
	/// </summary>
	void Release();

	/// <summary>
	/// テクスチャの取得
	/// </summary>
	/// <param name="a_key">ファイルパス</param>
	/// <returns>テクスチャのウィークポインタ</returns>
	bool GetTexture(Resource::ID& a_outID, const std::string& a_dir,const std::string& a_key, TextureUse a_use);

	/// <summary>
	/// テクスチャ取得
	/// </summary>
	/// <param name="a_texID">登録ID</param>
	/// <returns>テクスチャのポインタ</returns>
	const Texture* NGetTexture(const uint32_t& a_texID);

	const std::string& GetTexturePath(const uint32_t& a_texID);

	/// <summary>
	/// モデル取得
	/// </summary>
	/// <param name="a_key">ファイルパス</param>
	Resource::ID GetModel(const std::string& a_path);

	/// <summary>
	/// モデル取得
	/// </summary>
	/// <param name="a_modelID">登録ID</param>
	/// <returns>生ポインタ</returns>
	const Model* NGetModelResource(uint32_t a_modelID);
	Model* NGetModel(uint32_t a_modelID);

private:

	// モデルの読み込み
	void LoadModelFromPath(Model& a_model,const std::string& a_path);

private:

	ResourceSlotStorage<Texture> m_texStorage;

	ResourceSlotStorage<Model> m_modelStorage;

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