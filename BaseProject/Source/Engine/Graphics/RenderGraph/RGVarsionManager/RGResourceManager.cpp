#include "RGResourceManager.h"

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
		auto _it = m_resourceMap.find(a_name);
		if (_it != m_resourceMap.end())
		{
			return;
		}

		ResourceData _data = {};
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
		m_resourceMap[a_name] = _data;
	}

	RGResource RGResourceManager::Read(
		const std::string& a_resourceName,
		const Resource::TextureUsage& a_texUsage
	)
	{
		// 登録されているか検索
		auto _it = m_resourceMap.find(a_resourceName);
		if (_it != m_resourceMap.end())
		{
			// 最新のリソースを返す
			auto& _data = _it->second;
			_data.usage |= a_texUsage;	// 使用方法を追加
			return {a_resourceName , _data.currentVarsion};
		}
		return {};
	}

	RGResource RGResourceManager::Write(const std::string& a_resourceName, const Resource::TextureUsage& a_texUsage)
	{
		// 登録されているか検索
		auto _it = m_resourceMap.find(a_resourceName);
		if (_it != m_resourceMap.end())
		{
			// リソースのバージョンを上げて返す
			auto& _data = _it->second;
			_data.currentVarsion++;		// バージョンを上げる
			_data.usage |= a_texUsage;	// 使用方法を追加
			return { a_resourceName , _data.currentVarsion };
		}
		return {};
	}
}
