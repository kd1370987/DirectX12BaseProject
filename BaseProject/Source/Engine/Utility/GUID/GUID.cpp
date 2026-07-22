#include "GUID.h"

namespace Engine
{
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
			RpcStringFreeA(&str);
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