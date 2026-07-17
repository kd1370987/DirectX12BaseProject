#pragma once

namespace Engine::Resource
{
	class ShadingModelTableIO
	{
	public:

		/// <summary>
		/// ファイルパスからシェーディングモデルテーブルを読み込む
		/// </summary>
		/// <param name="a_path">パス</param>
		/// <returns>シェーディングモデルの実態</returns>
		static ShadingModelTable LoadFromFile(const std::string& a_path);

		/// <summary>
		/// シェーディングモデルテーブルを作成 : メタデータと空のファイルを作成
		/// </summary>
		/// <param name="a_path">ディレクトリ名</param>
		/// <param name="a_name">ファイルとシェーディングモデルテーブルのタイプネーム</param>
		static void Create(const std::string& a_path, const std::string& a_name);

	};
}