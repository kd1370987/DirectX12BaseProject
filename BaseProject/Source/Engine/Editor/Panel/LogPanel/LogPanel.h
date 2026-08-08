#pragma once

#include "../IPanel.h"

namespace Engine::Editor
{
	/// <summary>
	/// エンジンのログを表示するパネル
	///
	/// ログ本文は ImGuiTextBuffer へ追記していき、行頭のオフセットだけを別に持つ。
	/// 表示は ImGuiListClipper で見えている行だけを描くので、行数が増えても描画コストは変わらない。
	/// </summary>
	class LogPanel : public IPanel
	{
	public:

		LogPanel();
		~LogPanel() override = default;

		const char* GetName() const override { return "LogPanel"; };
		void OnDrawImGui(EditorContext& a_editContext) override;

		// ログのクリア
		void Clear();

		// ログの追加 : 書式と可変引数
		void AddLog(const char* a_fmt, ...);

		// 書式を解釈せずにそのまま追加する。
		// '%' を含む文字列を流し込むときはこちらを使う
		void AddLogRow(const char* a_text);

	private:

		// 追記された分から行頭オフセットを拾い、必要ならスクロールを予約する
		void UpdateOffsetsAndScroll(int a_oldSize);

	private:

		ImGuiTextBuffer m_textBuffer;
		ImGuiTextFilter m_textFilter;

		ImVector<int> m_lineOffsets;

		bool m_isAutoScroll = true;
		bool m_isScrollToBottom = false;
	};
}
