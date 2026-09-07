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

	// アセットグループ : タイプごとに分けるプロパティを作る中間素材
	// 初回にできたものを基準として集める
	struct AssetGroup
	{
		std::string					type = "";			// タイプ : 初回のものを入れる
		std::string					basePath = "";		// 拡張子なしのベースパス(実際の綴り)
		std::string					fileName = "";		// ファイル名
		std::vector<std::string>	extensions = {};	// そのベースパスに付いている拡張子
	};

	// ゲームに使用する外部アセットを管理する
	class AssetDatabase
	{
	public:

		//-----------------------------------------------------------------------------------------------------
		// 初期化
		//-----------------------------------------------------------------------------------------------------

		// アセットの上位フォルダと作成拡張子指定
		void Init(
			const std::string& a_assetFilePath,
			const std::string& a_metafileExtension
		);
		// 解放処理
		void Release();

		// 読み込みたい拡張子があれば追加
		void AddSupporedExtensions(const TypeExtension& a_data);

		// アセットフォルダ以下を検索して、すべてのアセットにメタファイルを作る
		void CreateMetaFileForAllAssets();

		// ランタイム用情報へと変換
		void CreateRuntimeData();

		//-----------------------------------------------------------------------------------------------------
		// 更新
		//-----------------------------------------------------------------------------------------------------

		// クロールディレクトリ以下を監視する。ランタイム中で行うが、エディターモードのみで実行すべき
		void Update();

		// すべてのメタファイルを削除して再構築する
		void RebuildAllMetaData();		

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

		/// <summary>
		/// GUIDからメタ情報を引く。見つからなくてもログを出さない
		/// </summary>
		/// <remarks>
		/// GetAssetProperty は見つからないとログを出すので、エディターのように
		/// 毎フレーム引く場所では使えない(参照が1つ切れているだけでログが埋まる)。
		/// 「引けたら出す・駄目なら出さない」を判断するためのもの。
		/// 返るのは m_assetMap の実体なので、アセットパネルが配っているものと同じポインタ。
		/// </remarks>
		AssetProperty* FindAssetProperty(const Engine::GUID& a_guid);
	private:

		//-----------------------------------------------------------------------------------------------------
		// ロックを取らない実装
		//
		// m_mutex は std::mutex なので同じスレッドから二重に取れない。
		// ロック済みの関数から公開関数を呼ぶと自分の解放を待って固まるため、
		// 処理をまたぐときは公開関数ではなくこちらを呼ぶこと。
		// 呼ぶ側が m_mutex を取っていることが前提。
		//-----------------------------------------------------------------------------------------------------

		// アセットフォルダ以下を検索して、すべてのアセットにメタファイルを作る
		void CreateMetaFileForAllAssetsInternal();

		// ランタイム用情報へと変換
		void CreateRuntimeDataInternal();

		// 別スレッドでファイルの変更を監視する関数
		void FileWatch();

		// 新たにメタファイルの内容を作成して返す
		nlohmann::json CreateMetaData(const std::filesystem::path& a_srcFile);

		// 管理対象拡張子かどうか調べる
		bool IsSupported(const std::filesystem::path& a_filepath);

		// アセットツリーの更新
		void RefreshAssetTree();

		// ファイル単体でのプロパティ操作
		void AddFileProperty(const std::filesystem::path& a_filePath);
		void RemoveFileProperty(const std::filesystem::path& a_filePath);
		void ChangeFileName(const std::filesystem::path& a_filePath);

		// ファイルパスの拡張子からアセットのタイプを検出
		std::string GetAssetType(const std::filesystem::path& a_filePath);

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

		// ランタイム中のディレクトリ管理

		std::thread m_fileWatcherThread;
		std::atomic_bool m_isWatching = false;
		std::mutex m_mutex;

		std::atomic_bool m_isDirtyDir = false;		// ディレクトリ以下で変更があったかどうか
		HANDLE m_dirHandle;				// ディレクトリハンドル

		// 中間バッファ
		std::array<std::byte, 64 * 1024> m_buffer;											// 変更点
		std::unordered_map<std::string, std::vector<AssetGroup>> m_typeAssetGroupTemp = {};	// 変更をためるバッファ
		std::unordered_map<Engine::GUID, AssetProperty> m_changedAssetPropMap = { };		// 既存に対しての変更点



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