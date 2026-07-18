#pragma once
namespace Engine::Resource
{
	class ActionStateMachineAssetIO
	{
	public:
		/// <summary>ファイルパスからの読み込み</summary>
		static ActionStateMachineAsset LoadFromFile(const std::string& a_path);

		/// <summary>作成 : メタファイルと空のファイルを作成</summary>
		/// <param name="a_path">ディレクトリ名</param>
		/// <param name="a_name">ファイルとステートマシンの名前</param>
		static void Create(const std::string& a_path, const std::string& a_name);
	};
}
