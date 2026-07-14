#pragma once
namespace Engine::Resource
{
	// タイプに対応する拡張子
	struct TypeExtension
	{
		// 追加
		void AddExtensions(const std::string& a_ext) { extensions.push_back(a_ext); }

		std::string type;						// タイプ
		std::vector<std::string> extensions;	// ベースとなる拡張子(.gltf,.fbxなど)
		std::vector<std::string> typeExt;		// 独自規格(.ob,.oj)
	};

	// アセット一つ当たりの情報 : メタ情報データ
	struct AssetProperty
	{
		// アセットで変わらない情報
		std::string type = "";								// アセットの種別
		Engine::GUID guid = {};								// GUID
		std::string fileName = "";							// ファイル名
		std::string filePath = "";							// 拡張子なしのベースパス

		std::vector<std::string> extensionsVec = {};		// アセットが持っている拡張子
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

		// アセットの上位フォルダと作成拡張子指定
		void Init(
			const std::string& a_assetFilePath,
			const std::string& a_metafileExtension
		);

		// 読み込みたい拡張子があれば追加
		void AddSupporedExtensions(const TypeExtension& a_data);

		// アセットフォルダ以下を検索して、すべてのアセットにメタファイルを作る
		void CreateMetaFileForAllAssets();

		// すべてのメタファイルを削除して再構築する
		void RebuildAllMetaData();

		// ランタイム用情報へと変換
		void CreateRuntimeData();

		// ランタイム中にファイルが追加された際に追加される
		Engine::GUID AddMetaData(const std::string& a_newFilePath,const std::string& a_type);

		// データベース内にGUIDが存在するかチェック
		bool IsValid(const Engine::GUID& a_guid) const;

		// ---- アクセサ ----
		std::string GetFilePathFromGUID(const std::string& a_guid);			// GUIDから現在のファイルパスを取得
		std::string GetFilePathFromGUID(const Engine::GUID& a_guid);		// GUIDから現在のファイルパスを取得
		std::string GetBaseFilePathFromGUID(const Engine::GUID& a_guid);	// ベースファイルパスの取得
		std::string GetFileNameFromGUID(const Engine::GUID& a_guid);		// ファイルネームの取得
		Engine::GUID GetGUIDFromFilePath(const std::string& a_path) const;		// ファイルパスからGUIDを取得
		const AssetNode& GetAssetRootNode() const { return m_assetRootNode; }		// アセット構造取得
		const std::unordered_map<std::string, TypeExtension>& GetAssetTypeExtensionsMap() const;
		std::span<const AssetProperty> GetTypeMetaVec(const std::string& a_type);		// 指定したタイプのメタ配列取得

		const AssetProperty* GetAssetProperty(const Engine::GUID& a_guid) const;
		const AssetProperty* GetAssetProperty(const std::string& a_filePath) const;
	private:

		// 新たにメタファイルの内容を作成して返す
		nlohmann::json CreateMetaData(const std::filesystem::path& a_srcFile);

		// 管理対象拡張子かどうか調べる
		bool IsSupported(const std::filesystem::path& a_filepath);

		// アセットツリーの更新
		void RefreshAssetTree();

	private:

		// ファイルパス
		std::string m_assetsFilePath = {};		// アセットが入っているフォルダ
		std::string m_metafileExtension = {};	// 作成するメタファイルの拡張子

		// 管理しているタイプと拡張子
		std::unordered_map<std::string, TypeExtension> m_assetTypeExtensionsMap;
		// 対応しているファイル拡張子ごとのメタデータ
		std::unordered_map<std::string, std::vector<AssetProperty>> m_typeMetaMap = {};
		// 管理しているすべてのアセットメタデータ
		std::unordered_map<Engine::GUID, AssetProperty> m_assetMap;

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