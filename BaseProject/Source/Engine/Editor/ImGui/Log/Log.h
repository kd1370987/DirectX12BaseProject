#pragma once
namespace Engine::Editor
{
	class Log
	{
	public:

		Log() = default;
		~Log() = default;

		void Init();

		void Clear();

		void AddLog(const char* a_fmt, ...);
		void AddLogRow(const char* a_text);

		void Draw(const char* a_title, bool* a_pOpen = NULL);

	private:

		void UpdateOffsetsAndScroll(int a_oldSize);

	private:

		ImGuiTextBuffer m_textBuffer;
		ImGuiTextFilter m_textFilter;

		ImVector<int> m_lineOffsets;

		bool m_isAutoScroll = true;
		bool m_isScrollToBottom = false;
	};
}