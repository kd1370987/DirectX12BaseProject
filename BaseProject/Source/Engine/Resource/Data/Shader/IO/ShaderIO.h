#pragma once
namespace Engine::Resource
{
	class ShaderIO
	{
	public:

		/// <summary>
		/// シェーダー読み込み
		/// </summary>
		/// <param name="a_path">ファイルパス</param>
		/// <returns>シェーダーの実態</returns>
		static Shader LoadShaderFromFile(const std::string& a_path);

		/// <summary>
		/// ライブラリシェーダーの読み込み
		/// </summary>
		/// <param name="a_path">ファイルパス</param>
		/// <returns>ライブラリシェーダーの実体</returns>
		static ShaderLibrary LoadShaderLibraryFromFile(const std::string& a_path);

		/// <summary>
		/// パスからのシェーダー読み込み
		/// </summary>
		/// <param name="a_path">パス</param>
		/// <returns>リソースマネージャーに登録されたハンドル</returns>
		static Handle<Shader> Request(const std::string& a_path);

		/// <summary>
		/// ライブラリシェーダーの読み込み
		/// </summary>
		/// <param name="a_path">パス</param>
		/// <returns>リソースマネージャーに登録されたハンドル</returns>
		static Handle<ShaderLibrary> RequestShaderLibrary(const std::string& a_path);
	};
}