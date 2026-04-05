#include "RGResourceManager.h"

#include "../../../Resource/Manager/TextureManager/TextureManager.h"

namespace Engine::Graphics
{
	void Engine::Graphics::RGResourceManager::Register(
		const std::string& a_name,
		const DXGI_FORMAT& a_format,
		const UINT64& a_widht,
		const UINT& a_height,
		const Resource::TextureUsage& a_texUsage
	)
	{
		auto _it = m_stringMap.find(a_name);
		if (_it != m_stringMap.end())
		{
			// 登録済み
			return;
		}

		LogicalResource _data = {};
		_data.name = a_name;
		_data.format = a_format;
		_data.widht = a_widht;
		_data.height = a_height;
		_data.usage = a_texUsage;

		// バージョンは0
		_data.currentVarsion = 0;

		// コンパイル時に作成されて渡される
		_data.texHandle = {};

		// 登録
		m_stringMap[a_name] = m_resourceVec.size();
		m_resourceVec.push_back(_data);
	}

	Resource::ID RGResourceManager::Read(
		const std::string& a_resourceName,
		const Resource::TextureUsage& a_texUsage
	)
	{
		// 登録されているか検索
		auto _it = m_stringMap.find(a_resourceName);
		if (_it != m_stringMap.end())
		{
			// 最新のリソースを返す
			auto& _data = m_resourceVec[_it->second];
			_data.usage |= a_texUsage;	// 使用方法を追加
			return Resource::GetID(_it->second,_data.currentVarsion);
		}
		return Resource::Limits::INVALID_ID;
	}

	Resource::ID RGResourceManager::Write(const std::string& a_resourceName, const Resource::TextureUsage& a_texUsage)
	{
		// 登録されているか検索
		auto _it = m_stringMap.find(a_resourceName);
		if (_it != m_stringMap.end())
		{
			// リソースのバージョンを上げて返す
			auto& _data = m_resourceVec[_it->second];
			_data.currentVarsion++;		// バージョンを上げる
			_data.usage |= a_texUsage;	// 使用方法を追加
			return Resource::GetID(_it->second, _data.currentVarsion);
		}
		return Resource::Limits::INVALID_ID;
	}
	void RGResourceManager::CreateAllTexture()
	{
		for (auto& _res : m_resourceVec)
		{
			Resource::CreateTextureDesc _desc = {
				.name = _res.name,
				.width = _res.widht,
				.height = _res.height,
				.format = _res.format,
				.usage = _res.usage
			};
			_res.texHandle = Resource::TextureManager::Instance().CreateTexture(_desc);
		}
	}
}
