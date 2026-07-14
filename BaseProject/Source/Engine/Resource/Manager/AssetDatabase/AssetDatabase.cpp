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
	void AssetDatabase::AddSupporedExtensions(const TypeExtension& a_data)
	{
		m_assetTypeExtensionsMap[a_data.type] = a_data;
	}
	Engine::GUID AssetDatabase::AddMetaData(const std::string& a_baseFilePath, const std::string& a_type)
	{
		// 拡張子なしの論理パス
		std::filesystem::path _basePath(a_baseFilePath);				// ファイルパス
		std::filesystem::create_directories(_basePath.parent_path());	// 親フォルダを作成

		// メタファイルパス作成
		std::filesystem::path _metaPath = _basePath.string() + m_metafileExtension;

		// メタファイルの作成（すでに存在していれば既存のGUIDを使う）
		nlohmann::json _json;
		Engine::GUID _guid;

		if (std::filesystem::exists(_metaPath))
		{
			// 既存のメタファイルがあればそこから取得
			std::ifstream _ifs(_metaPath.string());
			_ifs >> _json;
			_ifs.close(); // ★念のためcloseを追加
			_guid.FromString(JSONHelper::GetValue<std::string>("GUID", _json, Engine::DefaultGUID.String()));
		}
		else
		{
			// なければデータを作成して保存
			_guid.Create();
			_json["GUID"] = _guid.String();
			_json["Type"] = a_type;
			_json["Files"] = nlohmann::json::array();		// 手動追加時には空配列

			std::ofstream _metafile(_metaPath);
			_metafile << _json.dump(4);
			_metafile.close();
		}

		// -----------------------------------------------------
		// ランタイムデータの作成と更新
		// -----------------------------------------------------
		AssetProperty _prop;
		_prop.filePath = _basePath.lexically_normal().generic_string();
		_prop.fileName = _basePath.filename().string();
		// JSONからTypeを取得（なければ引数のa_type）
		_prop.type = JSONHelper::GetValue<std::string>("Type", _json, a_type);
		_prop.guid = _guid;

		// アセットが持っている拡張子リストの復元（CreateRuntimeDataと同じ動き）
		if (_json.contains("Files"))
		{
			for (const auto& _ext : _json["Files"])
			{
				_prop.extensionsVec.push_back(_ext.get<std::string>());
			}
		}

		// GUIDをキーにして登録（上書き更新）
		m_assetMap[_prop.guid] = _prop;

		// m_typeMetaMap の重複登録を防ぎつつ更新
		auto& _metaVec = m_typeMetaMap[_prop.type];
		auto _it = std::find_if(_metaVec.begin(), _metaVec.end(), [&_guid](const AssetProperty& p) { return p.guid == _guid; });
		if (_it != _metaVec.end())
		{
			*_it = _prop; // 既存なら上書き
		}
		else
		{
			_metaVec.push_back(_prop); // 新規なら追加
		}

		// -----------------------------------------------------
		// ツリー階層構造の再構築
		// -----------------------------------------------------
		RefreshAssetTree();

		return _prop.guid;
	}
	bool AssetDatabase::IsValid(const Engine::GUID& a_guid) const
	{
		if (m_assetMap.find(a_guid) != m_assetMap.end())
		{
			return true;
		}
		return false;
	}
	void AssetDatabase::CreateMetaFileForAllAssets()
	{
		// 拡張子なしのベースパスに付随する拡張子リスト
		std::map<std::string, std::vector<std::string>> _assetGroups;

		// スキャンしてベース名ごとに拡張子をグループ化
		for (const std::filesystem::directory_entry& _entry : std::filesystem::recursive_directory_iterator(m_assetsFilePath))
		{
			// エントリーがファイルかつサポートされた拡張子なら
			if (_entry.is_regular_file() && IsSupported(_entry.path()))
			{
				std::filesystem::path _basePath = _entry.path().parent_path() / _entry.path().stem();
				std::string _basePathStr = _basePath.lexically_normal().generic_string();				// 拡張子なしのベースパス

				// ベースパスのグループに追加
				_assetGroups[_basePathStr].push_back(_entry.path().extension().string());
			}
		}

		// 全ファイルがグループに登録されたため、グループごとのメタファイルを作成
		for (const auto& [_basePathStr, _extensions] : _assetGroups)
		{
			// 作成するメタファイルのフルパスを作成
			std::filesystem::path _metafilePath = _basePathStr + m_metafileExtension;

			// すでにメタファイルが存在する場合は、持っている拡張子リストのみ更新して保存
			nlohmann::json _json;
			if (std::filesystem::exists(_metafilePath))
			{
				std::ifstream _ifs(_metafilePath.string());
				if (_ifs.is_open())
				{
					_ifs >> _json;
					_ifs.close();
				}
			}
			else
			{
				Engine::GUID _guid;
				_guid.Create();
				_json["GUID"] = _guid.String();

				// 拡張子からタイプを推測
				std::string _typeStr = "Unknown";
				for (const auto& _ext : _extensions)
				{
					for (const auto& [_typeName, _typeExtData] : m_assetTypeExtensionsMap)
					{
						for (const auto& _tExt : _typeExtData.typeExt) {
							if (_ext.find(_tExt) == 0) { _typeStr = _typeName; break; }
						}
						for (const auto& _baseExt : _typeExtData.extensions) {
							if (_ext == _baseExt) { _typeStr = _typeName; break; }
						}
					}
					if (_typeStr != "Unknown") break;
				}
				_json["Type"] = _typeStr;
			}

			// 拡張子リストを常に最新の状態に更新
			auto _array = nlohmann::json::array();
			for (const auto& _ext : _extensions) { _array.push_back(_ext); }
			_json["Files"] = _array;

			std::ofstream _metafile(_metafilePath);
			_metafile << _json.dump(4);
			_metafile.close();
		}
	}

	void AssetDatabase::RebuildAllMetaData()
	{
		// ディレクトリが存在しない場合は処理をしない
		if (!std::filesystem::exists(m_assetsFilePath)) return;

		// 既存のメタファイルのみをすべて削除
		for (const auto& _entry : std::filesystem::recursive_directory_iterator(m_assetsFilePath))
		{
			// ファイルかつ拡張子がメタファイルのものだけを狙う
			if (_entry.is_regular_file() && _entry.path().extension().string() == m_metafileExtension)
			{
				std::error_code _ec;
				std::filesystem::remove(_entry.path(), _ec);

				// 万が一ファイルロック等で消せなかった場合のログ出力
				if (_ec)
				{
					Engine::Editor::MainEditor::Instance().AddLog(
						"DeleteMetaFile False : %s", _entry.path().string().c_str()
					);
				}
			}
		}

		// メタファイルを完全にクリーンな状態から作り直す
		CreateMetaFileForAllAssets();

		// ランタイムデータを最新のものに更新
		CreateRuntimeData();

		Engine::Editor::MainEditor::Instance().AddLog("All Asset Rebuild MetaData");
	}

	void AssetDatabase::CreateRuntimeData()
	{
		m_assetMap.clear();
		m_typeMetaMap.clear();

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
			try
			{
				_ifs >> _json;
			}
			catch (const nlohmann::json::parse_error&)
			{
				Engine::Editor::MainEditor::Instance().AddLog("ファイルオープンエラー : %s",_entry.path().string().c_str());
				continue;
			}

			// 現在のリソースパス（メタファイルの拡張子 .assetmeta を削除）
			auto _resPath = _entry.path();
			_resPath.replace_extension("");

			// 実体チェック
			//if (std::filesystem::exists(_resPath) == false) continue;

			// アセットプロパティの作成
			AssetProperty _property = {};

			// パスの正規化（ 区切り = / ）
			_property.filePath = _resPath.lexically_normal().generic_string();				// ファイルパス
			_property.fileName = _resPath.filename().string();								// ファイル名
			_property.type = JSONHelper::GetValue<std::string>("Type", _json, "Unknown");	// タイプの取得

			// GUIDの取得
			Engine::GUID _guid = {};
			std::string _default = _guid.String();
			_property.guid.FromString(JSONHelper::GetValue<std::string>("GUID", _json, _default));

			// アセットが持っている拡張子リストの復元
			if (_json.contains("Files"))
			{
				for (const auto& _ext : _json["Files"])
				{
					_property.extensionsVec.push_back(_ext.get<std::string>());
				}
			}

			// GUIDをキーにして登録
			m_assetMap[_property.guid] = _property;

			// 拡張子ごとにメタ配列を作成
			m_typeMetaMap[_property.type].push_back(_property);
		}

		// 階層構造の作成
		RefreshAssetTree();
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
		if (_it == m_assetMap.end()) return "";

		const AssetProperty& _prop = _it->second;

		// 最適な拡張子を探して結合する (.ob > .oj > 元データの順)
		auto _typeIt = m_assetTypeExtensionsMap.find(_prop.type);
		if (_typeIt != m_assetTypeExtensionsMap.end())
		{
			const auto& _typeData = _typeIt->second;

			// 1. 独自規格 (.ob系, .oj系) があれば優先して返す
			for (const auto& _tExt : _typeData.typeExt)
			{
				for (const auto& _ext : _prop.extensionsVec)
				{
					if (_ext.find(_tExt) == 0) return _prop.filePath + _ext;
				}
			}

			// 2. なければベース拡張子 (.gltf等) を返す
			for (const auto& _ext : _prop.extensionsVec)
			{
				for (const auto& _baseExt : _typeData.extensions)
				{
					if (_ext == _baseExt) return _prop.filePath + _ext;
				}
			}
		}

		// 万が一解決できなかった場合は、持っている最初の拡張子をとりあえず付ける
		if (!_prop.extensionsVec.empty())
		{
			return _prop.filePath + _prop.extensionsVec[0];
		}

		return _prop.filePath; // 最終手段
	}

	std::string AssetDatabase::GetBaseFilePathFromGUID(const Engine::GUID& a_guid)
	{
		auto _it = m_assetMap.find(a_guid);
		if (_it != m_assetMap.end())
		{
			return _it->second.filePath;
		}

		return "";
	}

	std::string AssetDatabase::GetFileNameFromGUID(const Engine::GUID& a_guid)
	{
		auto _it = m_assetMap.find(a_guid);
		if (_it != m_assetMap.end())
		{
			return _it->second.fileName;
		}

		return "";
	}

	Engine::GUID AssetDatabase::GetGUIDFromFilePath(const std::string& a_path) const
	{
		// 入力されたパスから拡張子を取り除き、ベースパスとして正規化する
		std::filesystem::path _inputPath(a_path);
		std::filesystem::path _basePath = _inputPath.parent_path() / _inputPath.stem();
		std::string _normBasePath = _basePath.lexically_normal().generic_string();

		for (auto& [_guid, _assetProp] : m_assetMap)
		{
			// 登録されているベースパスと比較
			if (_assetProp.filePath == _normBasePath)
			{
				return _guid;
			}
		}

		return Engine::DefaultGUID;
	}

	const std::unordered_map<std::string, TypeExtension>& AssetDatabase::GetAssetTypeExtensionsMap() const
	{
		return m_assetTypeExtensionsMap;
	}

	std::span<const AssetProperty> AssetDatabase::GetTypeMetaVec(const std::string& a_type)
	{
		auto _it = m_typeMetaMap.find(a_type);
		if (_it != m_typeMetaMap.end())
		{
			return _it->second;
		}
		return std::span<const AssetProperty>();
	}

	const AssetProperty* AssetDatabase::GetAssetProperty(const Engine::GUID& a_guid) const
	{
		auto _it = m_assetMap.find(a_guid);
		if (_it != m_assetMap.end())
		{
			return &_it->second;
		}

		ENGINE_LOG("アセットが見つかりませんでした : %s",a_guid.String().c_str());
		return nullptr;
	}

	const AssetProperty* AssetDatabase::GetAssetProperty(const std::string& a_filePath) const
	{
		return GetAssetProperty(GetGUIDFromFilePath(a_filePath));
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
		for (auto& [_type, _typeExt] : m_assetTypeExtensionsMap)
		{
			for (auto& _ext : _typeExt.extensions)
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

		// サポートされた拡張子と一致するならTrue
		for (auto& [_type, _typeExt] : m_assetTypeExtensionsMap)
		{
			// ベース拡張子のチェック (.gltf など)
			for (auto& _ext : _typeExt.extensions)
			{
				if (_fileExt == _ext) return true;
			}

			// 独自規格のチェック (.ob, .oj)
			for (auto& _tExt : _typeExt.typeExt)
			{
				if (_fileExt.find(_tExt) == 0) return true;
			}
		}

		// 対象がサポートされていない
		return false;
	}
	void AssetDatabase::RefreshAssetTree()
	{
		// 階層構造リセット
		m_assetRootNode.Clear();

		for (auto& [_type, _prop] : m_assetMap)
		{
			AssetNode* _pCurrent = &m_assetRootNode;
			std::filesystem::path _path(_prop.filePath);

			// ベースパスからの相対パスへの変換
			auto _relPath = std::filesystem::relative(_path, m_assetsFilePath);

			// フォルダ階層を辿る
			for (auto& _part : _relPath.parent_path())
			{
				std::string _folderName = _part.string();
				if (_folderName == "." || _folderName == "..") continue;
				_pCurrent = &_pCurrent->children[_folderName];
			}

			// アセットを追加
			_pCurrent->assets.push_back(&_prop);
		}

		return;
	}
}
