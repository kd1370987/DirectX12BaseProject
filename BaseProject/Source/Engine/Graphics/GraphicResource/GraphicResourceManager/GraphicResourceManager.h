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
	//==========================================================================================
	// 
	// マネージャーに対しての操作関連
	// 
	//==========================================================================================

	/// <summary>
	/// 初期化
	/// </summary>
	void Init();

	/// <summary>
	/// 解放
	/// </summary>
	void Release();

	//==========================================================================================
	// 
	// テクスチャ関連
	// 
	//==========================================================================================

	/// <summary>
	/// テクスチャの取得
	/// </summary>
	/// <param name="a_key">ファイルパス</param>
	/// <returns>テクスチャのウィークポインタ</returns>
	bool GetTexture(Engine::Resource::ID& a_outID, const std::string& a_dir,const std::string& a_key, TextureUse a_use);

	/// <summary>
	/// テクスチャ取得
	/// </summary>
	/// <param name="a_texID">登録ID</param>
	/// <returns>テクスチャのポインタ</returns>
	const Texture* NGetTexture(const uint32_t& a_texID);

	const std::string& GetTexturePath(const uint32_t& a_texID);

	//==========================================================================================
	// 
	// モデル
	//
	//==========================================================================================

	/// <summary>
	/// モデル取得
	/// </summary>
	/// <param name="a_key">ファイルパス</param>
	Engine::Resource::ID GetModel(const std::string& a_path);

	/// <summary>
	/// モデル取得
	/// </summary>
	/// <param name="a_modelID">登録ID</param>
	/// <returns>生ポインタ</returns>
	const Engine::Resource::Model* NGetModelResource(uint32_t a_modelID);
	Engine::Resource::Model* NGetModel(uint32_t a_modelID);

	UINT GetModelResourceStorageSize();

private:

	// モデルの読み込み
	void LoadModelFromPath(Engine::Resource::Model& a_model,const std::string& a_path);

private:

	ResourceSlotStorage<Texture> m_texStorage;

	ResourceSlotStorage<Engine::Resource::Model> m_modelStorage;

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