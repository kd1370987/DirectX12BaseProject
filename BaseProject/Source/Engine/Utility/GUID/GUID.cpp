#include "GUID.h"

namespace Engine
{
	//UUID CreateGUID()
	//{
	//	UUID _guid = {};
	//	UuidCreate(&_guid);
	//	return _guid;
	//}
	//std::string ToString(UUID a_guid)
	//{
	//	std::string  _ret = "None";
	//	RPC_CSTR _string;
	//	if (UuidToStringA(&a_guid, &_string) == RPC_S_OK)
	//	{
	//		_ret = (char*)_string;
	//	}
	//	return _ret;
	//}
	//UUID FromString(std::string a_stringGuid)
	//{
	//	UUID _guid = {};
	//	UuidFromStringA((RPC_CSTR)a_stringGuid.c_str(),&_guid);
	//	return _guid = {};
	//}

	void GUID::Create()
	{
		UuidCreate(&value);
	}
	std::string GUID::String() const
	{
		std::string ret = "None";
		RPC_CSTR str = nullptr;

		if (UuidToStringA(&value, &str) == RPC_S_OK)
		{
			ret = (char*)str;
			RpcStringFreeA(&str); // ← これ超重要
		}

		return ret;
	}
	void GUID::FromString(const std::string& a_str)
	{
		UuidFromStringA((RPC_CSTR)a_str.c_str(),&value);
	}
	size_t GUID::Hash() const noexcept
	{
		const uint64_t* p = reinterpret_cast<const uint64_t*>(&value);
		return std::hash<uint64_t>()(p[0]) ^ (std::hash<uint64_t>()(p[1]) << 1);
	}
}