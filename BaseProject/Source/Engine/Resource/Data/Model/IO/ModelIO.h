#pragma once
namespace Engine::Resource
{
	class ModelIO
	{
	public:
		// ---- Input ----
		static Model Import(const std::string& a_filePath);

	private:
		// 独自形式の読み込み処理
		static void Load(const std::string& a_filePath);

		// 優先順位の高い拡張子を検索
		static std::string FinddExtension(const std::vector<std::string>& a_extVed);
	};
}