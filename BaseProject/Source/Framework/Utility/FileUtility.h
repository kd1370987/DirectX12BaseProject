#pragma once

namespace FileUtility
{
	inline std::string GetFilePathExtension(const std::string& a_fileName)
	{
		if (a_fileName.find_last_of(".") != std::string::npos)
		{
			return a_fileName.substr(a_fileName.find_last_of(".") + 1);
		}
		return "";
	}
}