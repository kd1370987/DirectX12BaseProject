#pragma once

#include "../../../../../Internal/EditorContext.h"

namespace Engine::Editor::Inspector
{
	/// <summary>
	/// オーディオビヘイビアの編集・詳細表示
	/// 始動時・継続中・終了時それぞれに鳴らす音を割り当てる
	/// </summary>
	/// <param name="a_editContext">エディターコンテキスト</param>
	/// <param name="a_pBehavior">編集対象のオーディオビヘイビア</param>
	void AudioBehaviorEdit(EditorContext& a_editContext, Resource::AudioBehavior* a_pBehavior);
}
