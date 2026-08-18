#include "LogPanel.h"

namespace Engine::Editor
{
	LogPanel::LogPanel()
	{
		// m_lineOffsets は先頭行のオフセット(0)が入っている状態が初期状態になる。
		// ここを空のままにすると1行目が描画対象から漏れるので、必ず Clear を通す
		Clear();
	}

	void LogPanel::Clear()
	{
		m_textBuffer.clear();
		m_textFilter.Clear();
		m_lineOffsets.clear();
		m_lineOffsets.push_back(0);

		// 流し込み待ちも一緒に捨てる。
		// 残しておくとクリアした直後に古いログが復活してしまう
		{
			std::lock_guard _lock(m_pendingMutex);
			m_pendingVec.clear();
		}
	}

	void LogPanel::AddLog(const char* a_fmt, ...)
	{
		if (!a_fmt) return;

		// 書式を解決してから積む。
		// 本文への追記は描画スレッドが FlushPending() で行う
		char _buf[2048];

		va_list _args = nullptr;
		va_start(_args, a_fmt);
		vsnprintf(_buf, sizeof(_buf), a_fmt, _args);
		va_end(_args);

		AddLogRow(_buf);
	}

	void LogPanel::AddLogRow(const char* a_text)
	{
		if (!a_text) return;

		std::lock_guard _lock(m_pendingMutex);

		// 描かれないまま溜まり続けるのを防ぐ。古いものから捨てる
		if (m_pendingVec.size() >= PENDING_MAX)
		{
			m_pendingVec.erase(m_pendingVec.begin());
		}

		m_pendingVec.emplace_back(a_text);
	}

	void LogPanel::FlushPending()
	{
		// ロックしている間は本文へ触らない。
		// 一度手元へ移してから追記することで、他スレッドを待たせる時間を短くする
		std::vector<std::string> _logVec;
		{
			std::lock_guard _lock(m_pendingMutex);
			if (m_pendingVec.empty()) return;
			_logVec.swap(m_pendingVec);
		}

		for (const std::string& _log : _logVec)
		{
			const int _oldSize = m_textBuffer.size();

			// 書式は積む前に解決済み。'%' を解釈させないため append を使う
			m_textBuffer.append(_log.c_str());

			UpdateOffsetsAndScroll(_oldSize);
		}
	}

	void LogPanel::OnDrawImGui(EditorContext& a_editContext)
	{
		// ウィンドウの Begin/End は PanelManager 側が行うので、ここでは中身だけを描く

		// 他スレッドから来たログを本文へ流し込む。
		// 本文と行オフセットを触るのはここから下(描画スレッド)だけにする
		FlushPending();

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
	}

	void LogPanel::UpdateOffsetsAndScroll(int a_oldSize)
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
