#include "AssetManager.h"

namespace Engine::Resource
{
	void AssetManager::Init(
		const std::string& a_assetFilePath,
		const std::string& a_metafileExtension
	)
	{
		// 初期化
		m_assetsFilePath = a_assetFilePath;
		m_metafileExtension = a_metafileExtension;
	}
	void AssetManager::AddSupporedExtensions(const std::string& a_type, const std::string& a_extensions)
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
	void AssetManager::CreateMetaFileForAllAssets()
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

	void AssetManager::CreateRuntimeData()
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
		}
	}

	std::string AssetManager::GetFilePathFromGUID(const std::string& a_guid)
	{
		Engine::GUID _guid = {};
		_guid.FromString(a_guid);
		return GetFilePathFromGUID(_guid);
	}

	std::string AssetManager::GetFilePathFromGUID(const Engine::GUID& a_guid)
	{
		auto _it = m_assetMap.find(a_guid);
		if (_it != m_assetMap.end())
		{
			return _it->second.filePath;
		}

		return "NoFilePath";
	}

	nlohmann::json AssetManager::CreateMetaData(const std::filesystem::path& a_srcFile)
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
	bool AssetManager::IsSupported(const std::filesystem::path& a_filepath)
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
