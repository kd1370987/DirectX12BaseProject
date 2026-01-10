#include "ResourceManager.h"

#include "Engine/GPUResource/Texture/Texture.h"
#include "Engine/GPUResource/Model/Model.h"

void ResourceManager::Init()
{
}

std::weak_ptr<Texture> ResourceManager::GetTexture(const std::string& a_key)
{
	if (a_key.empty())
	{
		auto _tex = m_textureStorage.Get("WhiteTex");
		if (!_tex)
		{
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
		return _tex;
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

std::weak_ptr<ModelResource> ResourceManager::GetModel(const std::string& a_key)
{
	if (a_key.empty())
	{
		assert(0 && "ファイルパス未入力");
		return std::weak_ptr<ModelResource>{};
	}

	// モデルが読み込まれてるか確認
	auto _spModel = m_modelStorage.Get(a_key);
	if (!_spModel)
	{
		// モデルが読み込まれてなかったら読み込み開始
		_spModel = std::make_shared<ModelResource>();
		if (!_spModel->Load(a_key))
		{
			assert(0 && "モデル読み込みに失敗");
			return std::weak_ptr<ModelResource>{};
		}

		// ストレージに追加
		m_modelStorage.Add(a_key, _spModel);

		m_modelIDMap.emplace(a_key,m_count);
		m_count++;
	}

	return _spModel;
}

UINT ResourceManager::GetModelID(const std::string& a_key)
{
	auto _it = m_modelIDMap.find(a_key);
	if (_it != m_modelIDMap.end())
	{
		return _it->second;
	}
	return 1000;
}
