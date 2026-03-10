#pragma once

class TextureView
{
public:

	void Init();

	void Draw();

private:

	void DrawTextureView(Engine::Resource::Texture& a_Texture);
};