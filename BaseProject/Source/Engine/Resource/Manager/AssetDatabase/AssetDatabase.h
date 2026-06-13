#pragma once
namespace Engine::Resource
{
	// アセット一つ当たりの情報
	struct AssetProperty
	{
		Engine::GUID guid = {};			// GUID
		std::string fileName = "";		// ファイル名
		std::string filePath = "";		// ファイルパス
		std::string type = "";			// アセットの種別
	};

	// アセットの階層構造用ノード
	struct AssetNode
	{
		std::map<std::string, AssetNode> children;
		std::vector<AssetProperty*> assets;

		// リセット
		void Clear()
		{
			children.clear();
			assets.clear();
		}
	};

	// ゲームに使用する外部アセットを管理する
	class AssetDatabase
	{
	public:

		// 初期化
		// アセットの上位フォルダと作成拡張子指定
		void Init(
			const std::string& a_assetFilePath,
			const std::string& a_metafileExtension,
			const std::string& a_compiledDir
		);

		// 読み込みたい拡張子があれば追加
		void AddSupporedExtensions(const std::string& a_type,const std::string& a_extensions);
		// タイプに対応する独自拡張子追加
		void AddTypeExtensions(const std::string& a_type,const std::string& a_extensions);

		// ランタイム中にファイルが追加された際に追加される
		Engine::GUID AddMetaData(const std::string& a_newFilePath,const std::string& a_type);

		// アセットフォルダ以下を検索して、すべてのアセットにメタファイルを作る
		// すでにメタファイルがあれば無視
		void CreateMetaFileForAllAssets();

		// ランタイム用情報へと変換
		void CreateRuntimeData();

		// GUIDから現在のファイルパスを取得
		std::string GetFilePathFromGUID(const std::string& a_guid);
		std::string GetFilePathFromGUID(const Engine::GUID& a_guid);
		// ベースファイルパスの取得
		std::string GetBaseFilePathFromGUID(const Engine::GUID& a_guid);
		// ファイルネームの取得
		std::string GetFileNameFromGUID(const Engine::GUID& a_guid);
		// ファイルパスからGUIDを取得
		Engine::GUID GetGUIDFromFilePath(const std::string& a_path);

		// ---- アクセサ ----
		const std::unordered_map<std::string, std::vector<std::string>>& GetSupportedExtensionMap();
		const std::unordered_map<Engine::GUID, AssetProperty>& GetAssetMap();
		const std::unordered_map<std::string, std::vector<AssetProperty>>& GetTypeMetaMap();
		std::span<const AssetProperty> GetTypeMetaVec(const std::string& a_type);

		// アセット構造取得
		const AssetNode& GetAssetRootNode() const { return m_assetRootNode; }

		// 本番環境用データの構築
		// アセットから階層をコピーしてバイナリのみでデータを保存する
		void CompiledAssetData();

	private:

		// 新たにメタファイルの内容を作成して返す
		nlohmann::json CreateMetaData(const std::filesystem::path& a_srcFile);

		// 管理対象拡張子かどうか調べる
		bool IsSupported(const std::filesystem::path& a_filepath);

		// アセットツリーの更新
		void RefreshAssetTree();

	private:

		std::string m_assetsFilePath = {};		// アセットが入っているフォルダ
		std::string m_metafileExtension = {};	// 作成するメタファイルの拡張子

		std::string m_compiledDir = {};			// コンパイルデータの入っているディレクトリ

		// 対応しているファイル拡張子
		std::unordered_map<std::string,std::vector<std::string>> m_supportedExtensionsMap = {};

		// タイプごとに独自構造体に直した際の拡張子
		std::unordered_map<std::string, std::vector<std::string>> m_typeExtensionsMap = {};

		// 管理しているすべてのアセットメタデータ
		std::unordered_map<Engine::GUID, AssetProperty> m_assetMap;

		// 対応しているファイル拡張子ごとのメタデータ
		std::unordered_map<std::string, std::vector<AssetProperty>> m_typeMetaMap = {};

		// 階層構造
		AssetNode m_assetRootNode = {};

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