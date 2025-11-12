#pragma once

namespace FileUtility
{
	// ファイルの拡張子を取得
	inline std::string GetFilePathExtension(const std::string& a_fileName)
	{
		if (a_fileName.find_last_of(".") != std::string::npos)
		{
			return a_fileName.substr(a_fileName.find_last_of(".") + 1);
		}
		return "";
	}
	inline std::wstring GetFilePathExtension(const std::wstring& a_fileName)
	{
		auto _idx = a_fileName.rfind(L'.');
		return a_fileName.substr(_idx + 1, a_fileName.length() - _idx - 1);
	}

	// 拡張子を置き換える処理
	inline std::wstring ReplaceFilePathExtension(const std::wstring& a_fileName, const char* a_ext)
	{
		std::filesystem::path _p = a_fileName.c_str();
		return _p.replace_extension(a_ext).c_str();
	}

	// ファイルパスから、親ディレクトリまでのパスを取得
	inline std::string GetDirFromPath(const std::string& a_path)
	{
		const std::string::size_type _pos = std::max<signed>((signed)a_path.find_last_of('/'), (signed)a_path.find_last_of('\\'));
		return (_pos == std::string::npos) ? std::string() : a_path.substr(0, _pos + 1);
	}

	// ファイルの直属のフォルダを取得
	inline std::wstring GetDirectoryPath(const std::wstring& a_origin)
	{
		std::filesystem::path _p = a_origin.c_str();
		_p.remove_filename();
		return _p.wstring();
	}
}