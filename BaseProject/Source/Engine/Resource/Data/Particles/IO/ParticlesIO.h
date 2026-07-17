#pragma once
namespace Engine::Resource
{
	class ParticlesAssetIO
	{
	public:
		/// <summary>
		/// パーティクルの読み込み
		/// </summary>
		/// <param name="a_path">ファイルパス</param>
		/// <returns>パーティクルの実体</returns>
		static ParticlesAsset LoadFromFile(const std::string& a_path);

		/// <summary>
		/// パーティクル作成 : メタデータと空のファイルを作成
		/// </summary>
		/// <param name="a_path">ディレクトリ名</param>
		/// <param name="a_name">ファイルとパーティクルの名前</param>
		static void Create(const std::string& a_path, const std::string& a_name);
	};
}