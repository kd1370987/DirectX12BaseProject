#include "AssetDatabase.h"

namespace Engine::Resource
{
	void AssetDatabase::Init(
		const std::string& a_assetFilePath,
		const std::string& a_metafileExtension
	)
	{
		// 初期化
		m_assetsFilePath = a_assetFilePath;
		m_metafileExtension = a_metafileExtension;
	}
	void AssetDatabase::AddSupporedExtensions(const std::string& a_type, const std::string& a_extensions)
	{
		// 検索
		auto _it = m_supportedExtensionsMap.find(a_type);
		if (_it != m_supportedExtensionsMap.end())
		{
			// 見つかればサポートするものとして追加
			_it->second.push_back(a_extensions);
			return;
		}

		// なければ新たに作成して追加
		m_supportedExtensionsMap[a_type] = {};
		m_supportedExtensionsMap[a_type].push_back(a_extensions);
	}
	void AssetDatabase::AddTypeExtensions(const std::string& a_type, const std::string& a_extensions)
	{
		// 検索
		auto _it = m_typeExtensionsMap.find(a_type);
		if (_it != m_typeExtensionsMap.end())
		{
			// 見つかればサポートするものとして追加
			_it->second.push_back(a_extensions);
			return;
		}

		// なければ新たに作成して追加
		m_typeExtensionsMap[a_type] = {};
		m_typeExtensionsMap[a_type].push_back(a_extensions);
	}
	Engine::GUID AssetDatabase::AddMetaData(const std::string& a_baseFilePath, const std::string& a_ext, const std::string& a_type)
	{
		// フォルダ作成（なければ）
		std::filesystem::create_directories(a_baseFilePath);

		auto _fileName = FileUtility::GetFileName(a_baseFilePath);

		std::string _binPath = a_baseFilePath + "/" + _fileName + ".ob" + a_ext;
		std::string _jsonPath = a_baseFilePath + "/" + _fileName + ".oj" + a_ext;
		std::string _metaPath = _binPath + m_metafileExtension;

		// 実体ファイルの作成
		std::ofstream _binFile(_binPath, std::ios::binary);
		_binFile.close();
		std::ofstream _jsonFile(_jsonPath);
		_jsonFile.close();

		// メタファイルの作成
		nlohmann::json _json;
		Engine::GUID _guid;
		_guid.Create();
		_json["GUID"] = _guid.String();
		_json["Type"] = a_type;

		std::ofstream _metafile(_metaPath);
		_metafile << _json.dump(4);
		_metafile.close();

		// マップへ追加
		AssetProperty _prop;
		_prop.filePath = std::filesystem::path(a_baseFilePath).lexically_normal().generic_string() + "/" + _fileName;
		_prop.fileName = _fileName;
		_prop.type = a_type;
		_prop.guid = _guid;

		m_assetMap[_prop.guid] = _prop;
		m_typeMetaMap[_prop.type].push_back(_prop);

		return _prop.guid;
	}
	void AssetDatabase::CreateMetaFileForAllAssets()
	{
		// 指定フォルダ以下をクロール
		for (const std::filesystem::directory_entry& _entry : std::filesystem::recursive_directory_iterator(m_assetsFilePath))
		{
			// 通常ファイルかどうか、管理対象の拡張子かどうか
			if (_entry.is_regular_file() && IsSupported(_entry.path()))
			{
				std::string _filePath = _entry.path().string();

				// メタファイル名
				std::filesystem::path _metafilePath = _entry.path();
				_metafilePath.replace_filename(
					_entry.path().filename().string() + m_metafileExtension
				);

				// メタファイルがなければ作成
				if (!std::filesystem::exists(_metafilePath))
				{
					std::ofstream _metafile(_metafilePath);
					// メタデータの出力
					_metafile << CreateMetaData(_entry.path());
				}
			}
		}
	}

	void AssetDatabase::CreateRuntimeData()
	{
		m_assetMap.clear();
		
		// 指定フォルダ以下のメタファイルをすべて検索
		for (auto& _entry : std::filesystem::recursive_directory_iterator(m_assetsFilePath))
		{
			// 通常ファイルでない ・ メタファイルではない
			if (_entry.is_regular_file() == false) continue;
			if (_entry.path().extension().string() != m_metafileExtension) continue;

			// メタファイルの読み込み
			std::ifstream _ifs(_entry.path().string());
			if (_ifs.fail()) continue;
			nlohmann::json _json;
			_ifs >> _json;

			// アセットプロパティの作成
			AssetProperty _property = {};

			// 現在のリソースパス
			auto _resPath = _entry.path();
			_resPath.replace_extension(""); // メタファイルの拡張子を削除

			// 元のファイル（実体）が削除されていたら無視する
			if (std::filesystem::exists(_resPath) == false) continue;

			// パスの正規化（ 区切り = / ）
			_property.filePath = _resPath.lexically_normal().generic_string();
			_property.fileName = _resPath.filename().string();

			// タイプの取得
			_property.type = JSONHelper::GetValue<std::string>("Type",_json,"Unknown");
			
			// GUIDの取得
			Engine::GUID _guid = {};
			std::string _default = _guid.String();
			_property.guid.FromString(JSONHelper::GetValue<std::string>("GUID", _json, _default));

			// GUIDをキーにして登録
			m_assetMap[_property.guid] = _property;

			// 拡張子ごとにメタ配列を作成
			m_typeMetaMap[_property.type].push_back(_property);
		}
	}

	std::string AssetDatabase::GetFilePathFromGUID(const std::string& a_guid)
	{
		Engine::GUID _guid = {};
		_guid.FromString(a_guid);
		return GetFilePathFromGUID(_guid);
	}

	std::string AssetDatabase::GetFilePathFromGUID(const Engine::GUID& a_guid)
	{
		auto _it = m_assetMap.find(a_guid);
		if (_it != m_assetMap.end())
		{
			return _it->second.filePath;
		}

		return "NoFilePath";
	}

	std::string AssetDatabase::GetFileNameFromGUID(const Engine::GUID& a_guid)
	{
		auto _it = m_assetMap.find(a_guid);
		if (_it != m_assetMap.end())
		{
			return _it->second.fileName;
		}

		return "NoFilePath";
	}

	Engine::GUID AssetDatabase::GetGUIDFromFilePath(const std::string& a_path)
	{
		// 比較のために引数のパスを正規化
		std::string _normPath = std::filesystem::path(a_path).lexically_normal().generic_string();

		for (auto& [_guid,_assetProp] : m_assetMap)
		{
			if (_assetProp.filePath == _normPath)
			{
				return _guid;
			}
		}

		return Engine::DefaultGUID;
	}

	const std::unordered_map<std::string, std::vector<std::string>>& AssetDatabase::GetSupportedExtensionMap()
	{
		return m_supportedExtensionsMap;
	}

	const std::unordered_map<Engine::GUID, AssetDatabase::AssetProperty>& AssetDatabase::GetAssetMap()
	{
		return m_assetMap;
	}

	const std::unordered_map<std::string, std::vector<AssetDatabase::AssetProperty>>& AssetDatabase::GetTypeMetaMap()
	{
		return m_typeMetaMap;
	}

	const std::vector<AssetDatabase::AssetProperty>& AssetDatabase::GetTypeMetaVec(const std::string& a_type)
	{
		auto _it = m_typeMetaMap.find(a_type);
		if (_it != m_typeMetaMap.end())
		{
			return _it->second;
		}
		return {};
	}

	nlohmann::json AssetDatabase::CreateMetaData(const std::filesystem::path& a_srcFile)
	{
		nlohmann::json _json;

		// 新しいGUIDを発行
		Engine::GUID _guid = {};
		_guid.Create();
		_json["GUID"] = _guid.String();

		// ファイル拡張子からタイプを推測して保存
		std::string _srcExt = a_srcFile.extension().string();
		std::string _typeStr = "Unknown";

		// サポートされているか調べる
		for (auto& [_type, _extVec] : m_supportedExtensionsMap)
		{
			for (auto& _ext : _extVec)
			{
				if (_srcExt != _ext) continue;
				
				// 一致したら保存
				_typeStr = _type;
			}
		}

		// JSONに追加
		_json["Type"] = _typeStr;
		
		return _json;
	}
	bool AssetDatabase::IsSupported(const std::filesystem::path& a_filepath)
	{
		// 拡張子のみを抜き出して文字列化
		std::string _fileExt = a_filepath.extension().string();

		// サポートされた拡張しと一致するならTrue
		for (auto& [_type,_extVec] : m_supportedExtensionsMap)
		{
			for(auto& _ext : _extVec)
			{
				if (_fileExt == _ext)
				{
					return true;
				}
			}
		}
		
		// 対象がサポートされていない
		return false;
	}
}
