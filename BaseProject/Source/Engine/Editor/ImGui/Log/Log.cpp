#include "Log.h"
namespace Engine::Editor
{
	void Log::Init()
	{
		m_isAutoScroll = true;
		m_isScrollToBottom = false;
		Clear();
	}

	void Log::Clear()
	{
		m_textBuffer.clear();
		m_textFilter.Clear();
		m_lineOffsets.clear();
		m_lineOffsets.push_back(0);
	}

	void Log::AddLog(const char* a_fmt, ...)
	{
		// 前回のサイズを記録
		int _oldSize = m_textBuffer.size();

		// テキストバッファにデータを追加
		va_list _args = nullptr;
		va_start(_args, a_fmt);
		m_textBuffer.appendfv(a_fmt, _args);
		va_end(_args);

		UpdateOffsetsAndScroll(_oldSize);
	}

	void Log::AddLogRow(const char* a_text)
	{
		int _oldSize = m_textBuffer.size();

		// appendfv ではなく append を使う！（% を解釈しない）
		m_textBuffer.append(a_text);

		UpdateOffsetsAndScroll(_oldSize);
	}

	void Log::Draw(const char* a_title, bool* a_pOpen)
	{
		ImGui::Begin(a_title, a_pOpen);


		// オプションメニュー
		if (ImGui::BeginPopup("Options"))
		{
			// オートスクロール
			if (ImGui::Checkbox("Auto-scroll", &m_isAutoScroll))
			{
				if (m_isAutoScroll)
				{
					m_isScrollToBottom = true;
				}
			}
			ImGui::EndPopup();
		}

		// オプションボタン
		if (ImGui::Button("Options"))
		{
			ImGui::OpenPopup("Options");
		}
		ImGui::SameLine();
		// ログのクリア
		bool _isClear = ImGui::Button("ClearLog");
		ImGui::SameLine();
		// コピー
		bool _isCopy = ImGui::Button("Copy");

		// フィルター
		m_textFilter.Draw("Filter", -100.0f);
		ImGui::Separator();

		// スクロールバー
		ImGui::BeginChild("Scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		// 特定の処理を実行
		if (_isClear)
		{
			Clear();
		}
		if (_isCopy)
		{
			ImGui::LogToClipboard();
		}

		// ログテキスト
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		const char* _buf = m_textBuffer.begin();
		const char* _bufEnd = m_textBuffer.end();

		if (m_textFilter.IsActive())
		{
			// フィルターがかかっていれば、条件に合うもののみ表示
			for (int _lineNo = 0; _lineNo < m_lineOffsets.Size; ++_lineNo)
			{
				const char* _lineStart = _buf + m_lineOffsets[_lineNo];
				const char* _lineEnd =
					(_lineNo + 1 < m_lineOffsets.Size) ? (_buf + m_lineOffsets[_lineNo + 1] - 1) : _bufEnd;

				// フィルターと一致していれば表示
				if (m_textFilter.PassFilter(_lineStart, _lineEnd))
				{
					ImGui::TextUnformatted(_lineStart, _lineEnd);
				}
			}
		}
		else
		{
			ImGuiListClipper _clipper = {};
			_clipper.Begin(m_lineOffsets.Size);
			while (_clipper.Step())
			{
				for (int _lineNo = _clipper.DisplayStart; _lineNo < _clipper.DisplayEnd; ++_lineNo)
				{
					const char* _lineStart = _buf + m_lineOffsets[_lineNo];
					const char* _lineEnd =
						(_lineNo + 1 < m_lineOffsets.Size) ? (_buf + m_lineOffsets[_lineNo + 1] - 1) : _bufEnd;

					// 表示
					ImGui::TextUnformatted(_lineStart, _lineEnd);
				}
			}
			_clipper.End();
		}
		ImGui::PopStyleVar();

		if (m_isScrollToBottom)
		{
			ImGui::SetScrollHereY(1.0f);
		}
		m_isScrollToBottom = false;
		ImGui::EndChild();
		ImGui::End();
	}
	void Log::UpdateOffsetsAndScroll(int a_oldSize)
	{
		// 前回のサイズから増えた分の間に改行があれば改行する
		for (int _newSize = m_textBuffer.size(); a_oldSize < _newSize; ++a_oldSize)
		{
			if (m_textBuffer[a_oldSize] == '\n')
			{
				m_lineOffsets.push_back(a_oldSize + 1);
			}
		}

		// 自動スクロールがONならば、自動でスクロールする
		if (m_isAutoScroll)
		{
			m_isScrollToBottom = true;
		}
	}
}
