#pragma once
namespace Engine::Editor
{
	class TextureView
	{
	public:

		void Init();

		void Draw(UINT a_widht, UINT a_height);

	private:

		void DrawTextureView(Engine::Resource::Texture& a_Texture, UINT a_widht, UINT a_height);

		float m_minSize = 100.0f;
	};
}