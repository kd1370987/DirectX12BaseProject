#pragma once
namespace Engine::Editor
{
	class RenderGraphView
	{
	public:

		void Init();

		void Draw(UINT a_widht, UINT a_height);

	private:

		int m_currentSelected = 0;
		std::string m_rgTexName = "";
	};
}