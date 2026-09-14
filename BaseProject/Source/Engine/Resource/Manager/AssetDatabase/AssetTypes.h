#pragma once
//==========================================================================================
//
// AssetDatabase が配るデータの型
//
// アセットデータベース本体を読まずに「選択中のアセット」などを持てるように分けてある。
// ここには振る舞いを持たない構造体だけを置く
//
//==========================================================================================
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
}
