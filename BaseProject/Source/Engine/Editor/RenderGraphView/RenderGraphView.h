#pragma once

class RenderGraphView
{
public:

	void Init();

	void Draw();

private:

	int m_currentSelected = 0;
	std::string m_rgTexName = "";
};