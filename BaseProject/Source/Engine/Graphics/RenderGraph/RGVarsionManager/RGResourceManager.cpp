#include "RGResourceManager.h"

//#include "../../../Resource/Manager/TextureManager/TextureManager.h"
#include "../../../Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../Resource/Loader/Texture/TextureLoader.h"

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
		_data.texHandle[0] = {};

		// 登録
		m_stringMap[a_name] = m_resourceVec.size();
		m_resourceVec.push_back(_data);
	}

	void RGResourceManager::RegisterTemporal(const std::string& a_name, const DXGI_FORMAT& format, const UINT64& a_widht, const UINT& a_height, const Resource::TextureUsage& a_texUsage, const DXSM::Color& a_clerColor)
	{
		auto _it = m_stringMap.find(a_name);
		if (_it != m_stringMap.end())
		{
			// 登録済み
			return;
		}

		LogicalResource _data = {};
		_data.name = a_name;
		_data.format = format;
		_data.widht = a_widht;
		_data.height = a_height;
		_data.usage = a_texUsage;
		_data.clerColor = a_clerColor;

		// テンポラルオン
		_data.isTemporal = true;

		// バージョンは0
		_data.currentVarsion = 0;

		// コンパイル時に作成されて渡される
		_data.texHandle[0] = {};

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
			//_data.currentVarsion++;		// バージョンを上げる
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
			if(_res.isTemporal)
			{
				// テンポラル作成
				auto _name = _desc.name;
				_desc.name = _name + "_A";
				_res.texHandle[0] = Resource::TextureLoader::Create(_desc);
				_desc.name = _name + "_B";
				_res.texHandle[1] = Resource::TextureLoader::Create(_desc);
			}
			else
			{
				// 通常リソース作成
				_res.texHandle[0] = Resource::TextureLoader::Create(_desc);
			}
		}
	}
	void RGResourceManager::StateReset()
	{
		for (auto& _res : m_resourceVec)
		{
			_res.currentState[0] = D3D12_RESOURCE_STATE_COMMON;
			_res.currentState[1] = D3D12_RESOURCE_STATE_COMMON;
		}
	}
	void RGResourceManager::Swap()
	{
		m_temporalIndex = 1 - m_temporalIndex;
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
	Handle<Resource::Texture> RGResourceManager::GetTexHandle(Resource::ID a_id, bool isRead)
	{
		auto _idx = Resource::GetIndex(a_id);
		if (m_resourceVec[_idx].isTemporal)
		{
			// ★ Read(SRV等)なら過去、Write(UAV/RTV等)なら現在を返す
			return m_resourceVec[_idx].texHandle[isRead ? (1 - m_temporalIndex) : m_temporalIndex];
		}
		else
		{
			return m_resourceVec[_idx].texHandle[0];
		}
	}
	Handle<D3D12::RTV> RGResourceManager::GetRTVHandle(Resource::ID a_id)
	{
		const auto& _res = GetRes(a_id);
		const auto* _tex = GetTex(_res.texHandle[m_temporalIndex]);
		return _tex->GetRTV();
	}
	Handle<D3D12::DSV> RGResourceManager::GetDSVHandle(Resource::ID a_id)
	{
		const auto& _res = GetRes(a_id);
		const auto* _tex = GetTex(_res.texHandle[m_temporalIndex]);
		return _tex->GetDSV();
	}
	Handle<D3D12::DSV> RGResourceManager::GetReadOnlyDSVHandle(Resource::ID a_id)
	{
		const auto& _res = GetRes(a_id);
		const auto* _tex = GetTex(_res.texHandle[m_temporalIndex]);
		return _tex->GetReadOnlyDSV();
	}
	D3D12_RESOURCE_STATES& RGResourceManager::RefCurrentState(Resource::ID a_id, bool isRead)
	{
		auto& _res = RefRes(a_id);
		if (_res.isTemporal)
		{
			// ★ ステートも過去用と現在用ですり替える
			return _res.currentState[isRead ? (1 - m_temporalIndex) : m_temporalIndex];
		}
		else
		{
			return _res.currentState[0];
		}
	}
	DXGI_FORMAT RGResourceManager::GetDXGIFormat(Resource::ID a_id)
	{
		const auto& _res = GetRes(a_id);
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
	const RGResourceManager::LogicalResource& RGResourceManager::GetRes(Resource::ID a_id) const
	{
		auto _idx = Resource::GetIndex(a_id);
		return m_resourceVec[_idx];
	}
	RGResourceManager::LogicalResource& RGResourceManager::RefRes(Resource::ID a_id)
	{
		auto _idx = Resource::GetIndex(a_id);
		return m_resourceVec[_idx];
	}
	const Resource::Texture* RGResourceManager::GetTex(const Handle<Resource::Texture>& a_handle) const
	{
		return Resource::ResourceManager::Instance().Get(a_handle);
	}
	Resource::Texture* RGResourceManager::RefTex(const Handle<Resource::Texture>& a_handle)
	{
		return Resource::ResourceManager::Instance().Ref(a_handle);
	}
}
