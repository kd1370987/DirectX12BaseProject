#pragma once
namespace Engine::Resource
{
	class MeshIO
	{
	public:

		/// <summary>
		/// ファイルパスからのメッシュ生成
		/// </summary>
		/// <param name="a_path">パス</param>
		/// <param name="a_pContext">
		/// ビルドコンテキスト。
		/// 省略した場合はその場でバッチを開くため、メッシュ1個ごとにキューへの実行が走る。
		/// </param>
		/// <returns>メッシュ実体</returns>
		static Mesh LoadFromFile(const std::string& a_path, const ResourceBuildContext* a_pContext = nullptr);
	};
}
