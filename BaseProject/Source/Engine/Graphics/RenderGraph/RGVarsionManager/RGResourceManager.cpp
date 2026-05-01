#include "RGResourceManager.h"

#include "../../../Resource/Manager/TextureManager/TextureManager.h"

namespace Engine::Graphics
{
	void RGResourceManager::Register(const std::string& a_name, const DXGI_FORMAT& a_format, const UINT64& a_widht, const UINT& a_height, const Resource::TextureUsage& a_texUsage, const DXSM::Color& a_clerColor)
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
		_data.clerColor = a_clerColor;

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
			Resource::TextureCreateDesc _desc = {
				.name = _res.name,
				.width = _res.widht,
				.height = _res.height,
				.format = _res.format,
				.usage = _res.usage
			};
			_desc.opClerValue = _res.clerColor;
			_res.texHandle = Resource::TextureManager::Instance().CreateTexture(_desc);
		}
	}
	void RGResourceManager::StateReset()
	{
		for (auto& _res : m_resourceVec)
		{
			_res.currentState = D3D12_RESOURCE_STATE_COMMON;
		}
	}
	Resource::ID RGResourceManager::GetID(const std::string& a_name)
	{
		// 登録されているか検索
		auto _it = m_stringMap.find(a_name);
		if (_it != m_stringMap.end())
		{
			// リソースのバージョンを上げて返す
			return _it->second;
		}
	}
	Resource::Handle<Resource::Texture> RGResourceManager::GetTexHandle(Resource::ID a_id)
	{
		auto _idx = Resource::GetIndex(a_id);
		return m_resourceVec[_idx].texHandle;
	}
	Resource::Handle<D3D12::RTV> RGResourceManager::GetRTVHandle(Resource::ID a_id)
	{
		auto _idx = Resource::GetIndex(a_id);
		auto& _res = m_resourceVec[_idx];

		auto& _tex = Resource::TextureManager::Instance().GetTexture(_res.texHandle);
		return _tex.GetRTV();
	}
	Resource::Handle<D3D12::DSV> RGResourceManager::GetDSVHandle(Resource::ID a_id)
	{
		auto _idx = Resource::GetIndex(a_id);
		auto& _res = m_resourceVec[_idx];

		auto& _tex = Resource::TextureManager::Instance().GetTexture(_res.texHandle);
		return _tex.GetDSV();
	}
	D3D12_RESOURCE_STATES& RGResourceManager::RefCurrentState(Resource::ID a_id)
	{
		auto _idx = Resource::GetIndex(a_id);
		auto& _res = m_resourceVec[_idx];
		return _res.currentState;
	}
	DXGI_FORMAT RGResourceManager::GetDXGIFormat(Resource::ID a_id)
	{
		auto _idx = Resource::GetIndex(a_id);
		auto& _res = m_resourceVec[_idx];
		return _res.format;
	}
	std::vector<std::string> RGResourceManager::GetResourceNameVec()
	{
		std::vector<std::string> _nameVec = {};
		for (auto& [_name, _idx] : m_stringMap)
		{
			_nameVec.push_back(_name);
		}
		return _nameVec;
	}
}
