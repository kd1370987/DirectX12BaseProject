#include "GUID.h"

namespace Engine::GUID
{
	UUID CreateGUID()
	{
		UUID _guid = {};
		UuidCreate(&_guid);
		return _guid;
	}
	std::string ToString(UUID a_guid)
	{
		std::string  _ret = "None";
		RPC_CSTR _string;
		if (UuidToStringA(&a_guid, &_string) == RPC_S_OK)
		{
			_ret = (char*)_string;
		}
		return _ret;
	}
	UUID FromString(std::string a_stringGuid)
	{
		UUID _guid = {};
		UuidFromStringA((RPC_CSTR)a_stringGuid.c_str(),&_guid);
		return _guid = {};
	}
}