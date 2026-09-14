#pragma once
namespace Engine::Resource
{
	class MaterialIO
	{
	public:

		/// <summary>
		/// ファイルパスからのマテリアル生成
		/// </summary>
		/// <param name="a_path">パス</param>
		/// <returns>マテリアルの実体</returns>
		// a_pContext : 参照テクスチャを読み込むときに使う
		static Material LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext);

		/// <summary>
		/// アセットデータベースに登録
		/// </summary>
		/// <param name="a_path">保存するパス</param>
		/// <param name="a_data">std::move使用 実体</param>
		/// <returns>リソースマネージャーに登録されたハンドル</returns>
		//static Handle<Material> Register(const std::string& a_path,Material&& a_data);
	};
}