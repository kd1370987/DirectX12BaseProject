#include "ResourceManager.h"

#include "Engine/GPUResource/Texture/Texture.h"

void ResourceManager::Init()
{
}

std::weak_ptr<Texture> ResourceManager::GetTexture(const std::string& a_key)
{
	if (a_key.empty())
	{
		//assert(0 && "テクスチャのファイルパス不明");
		// 白色テクスチャを作成して返す
		auto _spTexture = std::make_shared<Texture>();
		if (!_spTexture->WhiteTexture())
		{
			// 読み込み失敗
			assert(0 && "テクスチャの読み込みに失敗");
			return std::weak_ptr<Texture>{};
		}

		m_textureStorage.Add("WhiteTex", _spTexture);

		return _spTexture;
	}

	// すでに読み込まれているか確認
	auto _spTexture = m_textureStorage.Get(a_key);
	if (_spTexture)
	{
		// すでに読み込まれている場合はそれを返す
		return _spTexture;
	}
	
	// テクスチャを新規作成して読み込む
	_spTexture = std::make_shared<Texture>();
	if (!_spTexture->Load(a_key))
	{
		// 読み込み失敗
		assert(0 && "テクスチャの読み込みに失敗");
		return std::weak_ptr<Texture>{};
	}

	// ストレージに登録
	m_textureStorage.Add(a_key, _spTexture);
	return _spTexture;
}
