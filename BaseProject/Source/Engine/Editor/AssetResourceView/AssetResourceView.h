#pragma once
namespace Engine::Resource
{
	struct AssetProperty;
}

namespace Engine::Editor
{
	class TextureView;

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

	private:

		std::unique_ptr<TextureView> m_upTextureView = nullptr;


		// 文字列キャッシュ
		char m_nameCach[256] = "";
		char m_pathCach[256] = "";

		// 現在選択中のアセットポインタ
		Resource::AssetProperty* m_pAssetPropCach = nullptr;
	};
}