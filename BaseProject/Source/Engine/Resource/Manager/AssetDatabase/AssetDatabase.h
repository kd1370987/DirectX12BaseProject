#pragma once
namespace Engine::Resource
{
	// ゲームに使用する外部アセットを管理する
	class AssetDatabase
	{
	public:

		// アセット一つ当たりの情報
		struct AssetProperty
		{
			Engine::GUID guid = {};			// GUID
			std::string fileName = "";		// ファイル名
			std::string filePath = "";		// ファイルパス
			std::string type = "";			// アセットの種別
			//bool HasArchive = false;
		};

	public:

		// 初期化
		// アセットの上位フォルダと作成拡張子指定
		void Init(
			const std::string& a_assetFilePath,
			const std::string& a_metafileExtension
		);

		// 読み込みたい拡張子があれば追加
		void AddSupporedExtensions(const std::string& a_type,const std::string& a_extensions);

		// ランタイム中にファイルが追加された際に追加される
		Engine::GUID AddMetaData(const std::string& a_newFilePath,const std::string& a_ext,const std::string& a_type);

		// アセットフォルダ以下を検索して、すべてのアセットにメタファイルを作る
		// すでにメタファイルがあれば無視
		void CreateMetaFileForAllAssets();

		// ランタイム用情報へと変換
		void CreateRuntimeData();

		// GUIDから現在のファイルパスを取得
		std::string GetFilePathFromGUID(const std::string& a_guid);
		std::string GetFilePathFromGUID(const Engine::GUID& a_guid);
		// ファイルパスからGUIDを取得
		Engine::GUID GetGUIDFromFilePath(const std::string& a_path);

		// アクセサ
		const std::unordered_map<std::string, std::vector<std::string>>& GetSupportedExtensionMap();
		const std::unordered_map<Engine::GUID, AssetProperty>& GetAssetMap();
		const std::unordered_map<std::string, std::vector<AssetProperty>>& GetTypeMetaMap();
		const std::vector<AssetProperty>& GetTypeMetaVec(const std::string& a_type);

	private:

		// 新たにメタファイルの内容を作成して返す
		nlohmann::json CreateMetaData(const std::filesystem::path& a_srcFile);

		// 管理対象拡張子かどうか調べる
		bool IsSupported(const std::filesystem::path& a_filepath);

	private:

		std::string m_assetsFilePath = {};		// アセットが入っているフォルダ
		std::string m_metafileExtension = {};	// 作成するメタファイルの拡張子

		// 対応しているファイル拡張子
		std::unordered_map<std::string,std::vector<std::string>> m_supportedExtensionsMap = {};

		// 管理しているすべてのアセットメタデータ
		std::unordered_map<Engine::GUID, AssetProperty> m_assetMap;

		// 対応しているファイル拡張子ごとのメタデータ
		std::unordered_map<std::string, std::vector<AssetProperty>> m_typeMetaMap = {};

	// シングルトン
	private:

		AssetDatabase() = default;
		~AssetDatabase() = default;

	public:

		static AssetDatabase& Instance()
		{
			static AssetDatabase _instance;
			return _instance;
		}

	};
}