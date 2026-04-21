#include "ComponentMetaRegistry.h"

namespace Engine::ECS
{

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

	const std::optional<SerializeFunc>& ComponentMetaRegistry::GetSerializeFunc(const ComponentTypeID& a_id) const
	{
		auto _it = m_compSerializeFuncMap.find(a_id);
		if (_it != m_compSerializeFuncMap.end())
		{
			return _it->second;
		}
		return std::optional<SerializeFunc>();
	}
	const std::optional<DeserializeFunc>& ComponentMetaRegistry::GetDeserializeFunc(const ComponentTypeID& a_id) const
	{
		auto _it = m_compDeserializeFuncMap.find(a_id);
		if (_it != m_compDeserializeFuncMap.end())
		{
			return _it->second;
		}
		return std::optional<DeserializeFunc>();
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