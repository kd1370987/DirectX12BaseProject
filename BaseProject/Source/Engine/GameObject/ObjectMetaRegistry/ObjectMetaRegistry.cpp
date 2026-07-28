#include "ObjectMetaRegistry.h"

namespace Engine::GameObject
{
	ObjectTypeID ObjectMetaRegistry::GetTypeID(const std::string& a_name) const
	{
		auto _it = m_nameMap.find(a_name);
		if (_it != m_nameMap.end())
		{
			return _it->second;
		}
		return INVALID_OBJECT_TYPE_ID;
	}

	ObjectTypeID ObjectMetaRegistry::GetTypeID(const std::type_index& a_index) const
	{
		auto _it = m_typeIndexMap.find(a_index);
		if (_it != m_typeIndexMap.end())
		{
			return _it->second;
		}
		return INVALID_OBJECT_TYPE_ID;
	}

	const ObjectMeta& ObjectMetaRegistry::GetMeta(ObjectTypeID a_id) const
	{
		auto _it = m_metaMap.find(a_id);
		if (_it != m_metaMap.end())
		{
			return _it->second;
		}

		// 未登録時は空メタを返す(静的なので寿命は安全)
		static const ObjectMeta _empty = {};
		return _empty;
	}

	std::unique_ptr<BaseObject> ObjectMetaRegistry::Create(ObjectTypeID a_id) const
	{
		auto _it = m_funcMap.find(a_id);
		if (_it != m_funcMap.end() && _it->second.create)
		{
			return _it->second.create();
		}
		return nullptr;
	}
}
