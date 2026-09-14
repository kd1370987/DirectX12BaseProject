#pragma once
namespace Engine::Resource
{
	class AssetDatabase;
	class ResourceManager;

	class EffectAssetIO
	{
	public:

		/// <summary>
		/// ファイルパスからの読み込み
		/// </summary>
		/// <param name="a_path">ファイルパス</param>
		/// <returns>実体を返す</returns>
		static EffectAsset LoadFromFile(const std::string& a_path, ResourceManager& a_resourceManager);

		/// <summary>
		/// 作成 : メタファイルと空のファイルを作成
		/// </summary>
		/// <param name="a_path">Asset/Effect/ 以下のディレクトリ名</param>
		/// <param name="a_name">ファイルとエフェクトの名前</param>
		static void Create(AssetDatabase& a_assetDB, const std::string& a_path, const std::string& a_name);
	};
}
