#pragma once

#include "../IPanel.h"

namespace Engine::Editor
{
	class AssetDataBasePanel : public IPanel
	{
	public:
		AssetDataBasePanel();
		~AssetDataBasePanel() override = default;

		const char* GetName() const override { return "AssetDataBase"; };
		void OnDrawImGui(EditorContext& a_editContext) override;
	private:

		void CreateAssetButton(EditorContext& a_editContext);

		void AssetDataBaseExplorer(EditorContext& a_editContext);

		float m_windowWidth = 1920;
		float m_windowHeight = 1080;

		// 文字列キャッシュ
		char m_nameCach[256] = "";
		char m_pathCach[256] = "";

		// "アセット名" -> "生成処理" の辞書
		using AssetCreateFunc = std::function<void(const std::string&, const std::string&)>;
		std::unordered_map<std::string, AssetCreateFunc> m_assetCreateFuncs;
	};
}