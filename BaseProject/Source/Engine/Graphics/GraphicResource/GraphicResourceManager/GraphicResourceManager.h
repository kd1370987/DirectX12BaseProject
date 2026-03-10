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
	const TextureS* NGetTexture(const uint32_t& a_texID);

	const std::string& GetTexturePath(const uint32_t& a_texID);

private:

	ResourceSlotStorage<TextureS> m_texStorage;


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