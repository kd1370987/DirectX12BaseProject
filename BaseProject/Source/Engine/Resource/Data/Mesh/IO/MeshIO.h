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
		/// <returns>メッシュ実体</returns>
		static Mesh LoadFromFile(const std::string& a_path);
	};
}