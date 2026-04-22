#include "ComponentMetaRegistry.h"

namespace Engine::ECS
{
	ComponentTypeID ComponentMetaRegistry::GetTypeID(const std::string& a_name)
	{
		auto _it = m_compNameMap.find(a_name);
		if (_it != m_compNameMap.end())
		{
			return _it->second;
		}
		return Limits::INVALID_COMPONENTTYPEID;
	}
	ComponentTypeID ComponentMetaRegistry::GetTypeID(const std::type_index& a_index) const
	{
		auto _it = m_typeIndexMap.find(a_index);
		if (_it != m_typeIndexMap.end())
		{
			return _it->second;
		}
		return Limits::INVALID_COMPONENTTYPEID;
	}

	const ComponentMeta& ComponentMetaRegistry::GetMetaData(const ComponentTypeID& a_id) const
	{
		auto _it = m_compTypeMap.find(a_id);
		if (_it != m_compTypeMap.end())
		{
			return _it->second;
		}

		assert(0 && "登録していないコンポーネントです");
		ComponentMeta _meta = {};
		return _meta;
	}

	const ComponentMeta& ComponentMetaRegistry::GetMetaData(const std::type_index& a_index) const
	{
		auto _it = m_typeIndexMap.find(a_index);
		if (_it != m_typeIndexMap.end())
		{
			return GetMetaData(_it->second);
		}

		assert(0 && "登録していないコンポーネントです");
		ComponentMeta _meta = {};
		return _meta;
	}

	const std::unordered_map<ComponentTypeID, ComponentMeta>& ComponentMetaRegistry::GetAllMetaData() const
	{
		return m_compTypeMap;
	}

	const ComponentFunc& ComponentMetaRegistry::GetFunc(const ComponentTypeID& a_id) const
	{
		auto _it = m_compFuncMap.find(a_id);
		if (_it != m_compFuncMap.end())
		{
			return _it->second;
		}
		return {};
	}
}