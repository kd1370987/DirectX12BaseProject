#pragma once

class Log
{
public:

	Log() = default;
	~Log() = default;

	void Init();

	void Clear();

	void AddLog(const char* a_fmt, ...);

	void Draw(const char* a_title,bool* a_pOpen = NULL);

private:

	ImGuiTextBuffer m_textBuffer;
	ImGuiTextFilter m_textFilter;

	ImVector<int> m_lineOffsets;

	bool m_isAutoScroll = true;
	bool m_isScrollToBottom = false;
};