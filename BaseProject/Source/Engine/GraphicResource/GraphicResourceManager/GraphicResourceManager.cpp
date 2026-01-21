#include "GraphicResourceManager.h"

#include "../Resource/Texture/Texture.h"
#include "../Resource/Model/Model.h"

void GraphicResourceManager::Init()
{
}

std::weak_ptr<Texture> GraphicResourceManager::GetTexture(const std::string& a_key)
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
	if (!_spTexture->NormalMapLoad(a_key))
	{
		// 読み込み失敗
		assert(0 && "テクスチャの読み込みに失敗");
		return std::weak_ptr<Texture>{};
	}

	// ストレージに登録
	m_textureStorage.Add(a_key, _spTexture);
	return _spTexture;
}

const ModelID& GraphicResourceManager::GetModel(const std::string& a_path)
{
	if (a_path.empty())
	{
		assert(0 && "ファイルパス未入力");
		return INVALID_MODEL_ID;
	}

	// モデルが読み込まれてるか確認
	auto _it = m_modelIDMap.find(a_path);
	if (_it != m_modelIDMap.end())
	{
		return _it->second;
	}
	else
	{
		// モデルが読み込まれてなかったら読み込み開始
		auto _spModel = std::make_shared<ModelResource>();
		if (!_spModel->Load(a_path))
		{
			assert(0 && "モデル読み込みに失敗");
			return INVALID_MODEL_ID;
		}

		// ストレージに追加
		m_modelIDStorage.Add(m_count, _spModel);
		m_modelIDMap.emplace(a_path, m_count);

		m_count++;
	}

	return m_modelIDMap[a_path];
}

ModelResource* GraphicResourceManager::NGetModelResource(UINT a_modelID)
{
	auto _spModel = m_modelIDStorage.Get(a_modelID);
	if (_spModel)
	{
		return _spModel.get();
	}

	return nullptr;
}
