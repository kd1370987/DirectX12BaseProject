#pragma once
namespace Engine::Resource
{
	class AssetDatabase;
	class ResourceManager;

	class ParticlesAssetIO
	{
	public:
		/// <summary>
		/// パーティクルの読み込み
		/// </summary>
		/// <param name="a_path">ファイルパス</param>
		/// <returns>パーティクルの実体</returns>
		static ParticlesAsset LoadFromFile(const std::string& a_path, ResourceManager& a_resourceManager);

		/// <summary>
		/// パーティクル作成 : メタデータと空のファイルを作成
		/// </summary>
		/// <param name="a_path">ディレクトリ名</param>
		/// <param name="a_name">ファイルとパーティクルの名前</param>
		static void Create(AssetDatabase& a_assetDB, const std::string& a_path, const std::string& a_name);
	};
}