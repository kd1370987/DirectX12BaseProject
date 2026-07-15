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
		static Model Load(const std::string& a_filePath);
		// GLTFのロード
		static Model GLTFLoad(const std::string& a_filePath);



		// ランタイムデータの作成
		static void CreateDrawCmd(const ModelAssetData& a_modelAssetData,ModelRuntimeData& a_runtimeData);

		// 優先順位の高い拡張子を検索
		static std::string FinddExtension(const std::vector<std::string>& a_extVed);
	};
}