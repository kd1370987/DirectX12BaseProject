#pragma once
namespace Engine::Resource
{
	struct AssetProperty;
}

namespace Engine::Editor
{
	class AssetResourceView
	{
	public:

		AssetResourceView();
		~AssetResourceView();

		void Init();

		void Draw(UINT a_widht, UINT a_height);

	private:

		// 拡張子ごとの配列
		void ExtensionVec();

		// アセットデータベース描画
		void AssetDataBaseDraw();

		// 単体リソースビュー
		void ResourceView();

		// 各リソース描画
		void DrawModel();
		void DrawStateMachin();
		void DrawParticlesAsset();

		// 各リソース作成ボタン
		void CreateAssetButton();

	private:
		// 文字列キャッシュ
		char m_nameCach[256] = "";
		char m_pathCach[256] = "";

		// 現在選択中のアセットポインタ
		Resource::AssetProperty* m_pAssetPropCach = nullptr;
	};
}