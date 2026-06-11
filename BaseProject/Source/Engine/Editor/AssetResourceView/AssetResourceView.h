#pragma once
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

		// リソースビューの作成
		void ResourceView();

		// 拡張子ごとの配列
		void ExtensionVec();

	private:

		std::unique_ptr<TextureView> m_upTextureView = nullptr;


		// 文字列キャッシュ
		char m_nameCach[256] = "";
		char m_pathCach[256] = "";
	};
}