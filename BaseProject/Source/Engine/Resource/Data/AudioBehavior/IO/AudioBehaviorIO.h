#pragma once
namespace Engine::Resource
{
	class AudioBehaviorIO
	{
	public:

		/// <summary>
		/// ファイルパスからの読み込み
		/// </summary>
		/// <param name="a_path">ファイルパス</param>
		/// <returns>実体を返す</returns>
		static AudioBehavior LoadFromFile(const std::string& a_path);

		/// <summary>
		/// 作成 : メタファイルと空のファイルを作成
		/// </summary>
		/// <param name="a_path">Asset/AudioBehavior/ 以下のディレクトリ名</param>
		/// <param name="a_name">ファイルとビヘイビアの名前</param>
		static void Create(const std::string& a_path, const std::string& a_name);
	};
}
