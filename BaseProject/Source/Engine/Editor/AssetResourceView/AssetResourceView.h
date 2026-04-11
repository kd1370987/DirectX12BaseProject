#pragma once
namespace Engine::Editor
{
	class ModelView;
	class TextureView;

	class AssetResourceView
	{
	public:

		AssetResourceView();
		~AssetResourceView();

		void Init();

		void Draw();

	private:

		std::unique_ptr<ModelView> m_upModelView = nullptr;
		std::unique_ptr<TextureView> m_upTextureView = nullptr;

	};
}