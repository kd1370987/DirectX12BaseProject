#include "AssetDatabase.h"

namespace Engine::Resource
{
	void AssetDatabase::Init(
		const std::string& a_assetFilePath,
		const std::string& a_metafileExtension,
		const std::string& a_compiledDir
	)
	{
		// 初期化
		m_assetsFilePath = a_assetFilePath;
		m_metafileExtension = a_metafileExtension;
		m_compiledDir = a_compiledDir;
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
	Engine::GUID AssetDatabase::AddMetaData(const std::string& a_baseFilePath, const std::string& a_type)
	{
		// 拡張子なしの論理パス
		std::filesystem::path _basePath(a_baseFilePath);
		std::filesystem::create_directories(_basePath.parent_path()); // 親フォルダを作成

		// メタファイル名
		std::filesystem::path _metaPath = _basePath.string() + m_metafileExtension;

		// メタファイルの作成（すでに存在していれば既存のGUIDを使う）
		nlohmann::json _json;
		Engine::GUID _guid;

		if (std::filesystem::exists(_metaPath))
		{
			std::ifstream _ifs(_metaPath.string());
			_ifs >> _json;
			_guid.FromString(JSONHelper::GetValue<std::string>("GUID", _json, Engine::DefaultGUID.String()));
		}
		else
		{
			_guid.Create();
			_json["GUID"] = _guid.String();
			_json["Type"] = a_type;

			std::ofstream _metafile(_metaPath);
			_metafile << _json.dump(4);
			_metafile.close();
		}

		// マップへ追加
		AssetProperty _prop;
		_prop.filePath = _basePath.lexically_normal().generic_string();
		_prop.fileName = _basePath.filename().string();
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
				Engine::Editor::MainEditor::Instance().AddLog(
					"ファイルオープンエラー : %s",_entry.path().string().c_str());
				continue;
			}

			// 現在のリソースパス（メタファイルの拡張子 .assetmeta を削除）
			// 例1: Player.fbx.assetmeta    -> Player.fbx
			// 例2: Player.obmesh.assetmeta -> Player.obmesh
			auto _resPath = _entry.path();
			_resPath.replace_extension("");

			// 実体チェック
			if (std::filesystem::exists(_resPath) == false) continue;

			// アセットプロパティの作成
			AssetProperty _property = {};

			// パスの正規化（ 区切り = / ）
			_property.filePath = _resPath.lexically_normal().generic_string();
			_property.fileName = _resPath.filename().string();

			// タイプの取得
			_property.type = JSONHelper::GetValue<std::string>("Type", _json, "Unknown");

			// GUIDの取得
			Engine::GUID _guid = {};
			std::string _default = _guid.String();
			_property.guid.FromString(JSONHelper::GetValue<std::string>("GUID", _json, _default));

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
		if (_it != m_assetMap.end())
		{
			return _it->second.filePath;
		}

		return "NoFilePath";
	}

	std::string AssetDatabase::GetBaseFilePathFromGUID(const Engine::GUID& a_guid)
	{
		auto _it = m_assetMap.find(a_guid);
		if (_it != m_assetMap.end())
		{
			// 拡張子のない形にして返す
			auto _dir = FileUtility::GetDirFromPath(_it->second.filePath);
			auto _name = FileUtility::GetFileNameWithoutExtension(_it->second.fileName);
			return _dir + _name;
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

	const std::unordered_map<Engine::GUID, AssetProperty>& AssetDatabase::GetAssetMap()
	{
		return m_assetMap;
	}

	const std::unordered_map<std::string, std::vector<AssetProperty>>& AssetDatabase::GetTypeMetaMap()
	{
		return m_typeMetaMap;
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

	void AssetDatabase::CompiledAssetData()
	{
		// ※事前に m_importedFilePath (例: "Imported") がInit等で設定されている想定
		std::filesystem::path _srcBase(m_assetsFilePath);
		std::filesystem::path _destBase(m_compiledDir);

		std::error_code _ec; // エラーキャッチ用

		// インポート先の大元フォルダがなければ作成
		if (!std::filesystem::exists(_destBase))
		{
			std::filesystem::create_directories(_destBase, _ec);
		}

		// Assetsフォルダ以下を再帰的にクロール
		for (const auto& _entry : std::filesystem::recursive_directory_iterator(_srcBase))
		{
			// Assetsルートからの相対パスを取得 (例: "Player/PlayerAI.obstate")
			std::filesystem::path _relPath = std::filesystem::relative(_entry.path(), _srcBase);

			// コピー先のフルパスを構築 (例: "Imported/Player/PlayerAI.obstate")
			std::filesystem::path _destPath = _destBase / _relPath;

			// フォルダの場合：階層をミラーリングして作成
			if (_entry.is_directory())
			{
				std::filesystem::create_directories(_destPath, _ec);
				continue;
			}

			// ファイルの場合：独自のバイナリ規格 (.ob系) かどうかを判定してコピー
			if (_entry.is_regular_file())
			{
				std::string _ext = _entry.path().extension().string();

				// 拡張子が ".ob" で始まる場合
				if (_ext.find(".ob") == 0)
				{
					// コピー先の親フォルダを念のため作成
					std::filesystem::create_directories(_destPath.parent_path(), _ec);

					// --- 手動でタイムスタンプを比較してコピーを制御 ---
					bool _shouldCopy = true;
					if (std::filesystem::exists(_destPath))
					{
						auto _srcTime = std::filesystem::last_write_time(_entry.path(), _ec);
						auto _destTime = std::filesystem::last_write_time(_destPath, _ec);

						// コピー先の方が新しい、または同じ時間ならスキップ
						if (_srcTime <= _destTime)
						{
							_shouldCopy = false;
						}
					}

					// コピー実行
					if (_shouldCopy)
					{
						// overwrite_existing を使って強制上書き
						bool _success = std::filesystem::copy_file(
							_entry.path(),
							_destPath,
							std::filesystem::copy_options::overwrite_existing,
							_ec // 例外ではなくエラーコードで受け取る
						);

						// ログ出力（デバッグ用）
						if (!_success || _ec)
						{
							Engine::Editor::MainEditor::Instance().AddLog(
								"Copy False: %s (sorce: %s)\n",
								_destPath.string().c_str(),
								_ec.message().c_str()
							);
						}
						else
						{
							Engine::Editor::MainEditor::Instance().AddLog(
								"Copy True: %s\n",
								_destPath.string().c_str()
							);
						}
					}
				}
			}
		}
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
