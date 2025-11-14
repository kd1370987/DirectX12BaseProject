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

	// ファイルの拡張子を除いた名前を取得
	inline std::string GetFileNameWithoutExtension(const std::string& a_fileName)
	{
		std::filesystem::path _p = a_fileName;
		return _p.stem().string();
	}
	inline std::wstring GetFileNameWithoutExtension(const std::wstring& a_fileName)
	{
		std::filesystem::path _p = a_fileName.c_str();
		return _p.stem().c_str();
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
	inline std::wstring GetDirectoryPath(const std::wstring& a_origin)
	{
		std::filesystem::path _p = a_origin.c_str();
		_p.remove_filename();
		return _p.wstring();
	}

	// 指定ディレクトリ内の指定拡張子のファイルを全て取得
	inline std::vector<std::filesystem::path> FindExtensionInDirectory(
		const std::filesystem::path& a_dirPath, 
		const std::string& a_ext
	)
	{
		// 出力用パス配列
		std::vector<std::filesystem::path> _outPaths;

		// ディレクトリが存在するかつディレクトリである場合
		if (std::filesystem::exists(a_dirPath) && std::filesystem::is_directory(a_dirPath))
		{
			// ディレクトリ内を走査
			for (const auto& entry : std::filesystem::directory_iterator(a_dirPath))
			{
				// 拡張子が一致したら出力用配列に追加
				if (entry.path().extension() == a_ext)
				{
					_outPaths.push_back(entry.path());
				}
			}
		}

		// パス配列を返す
		return _outPaths;
	}

	// ファイルの存在チェック
	inline bool IsExistFile(const std::string& a_filePath)
	{
		std::ifstream _file(a_filePath);
		bool _isOpen = _file.is_open();
		_file.close();
		return _isOpen;
	}
}