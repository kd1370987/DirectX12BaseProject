#include "ComponentMetaRegistry.h"

ECS::ComponentTypeID ComponentMetaRegistry::GetTypeID(const std::type_index& a_index) const
{
	auto _it = m_typeIndexMap.find(a_index);
	if (_it != m_typeIndexMap.end())
	{
		return _it->second;
	}
	return ECS::Limits::INVALID_COMPONENTTYPEID;
}

const ComponentMeta& ComponentMetaRegistry::GetMetaData(const ECS::ComponentTypeID& a_id) const
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
