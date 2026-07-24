#include "AnimatorEdit.h"

namespace Engine::Editor::Inspector
{
	//-----------------------------------------------------------------------------------------
	// アニメーター(アニメ用ステートマシン)の編集・詳細表示
	//-----------------------------------------------------------------------------------------
	void AnimatorEdit(
		EditorContext& a_editContext,
		Resource::AnimatorAsset* a_pAnimator,
		const Handle<Resource::AnimatorAsset>& a_handle
	)
	{
		if (!a_pAnimator) { return; }

		// ---- 概要 ----
		ImGui::Text("Name             : %s", a_pAnimator->GetName().c_str());

		// 開始ステート名 : ハッシュから引けなければハッシュのまま表示
		UINT _defaultStartHash = a_pAnimator->GetDefaultStartHash();
		auto _startName = a_pAnimator->GetNodeName(_defaultStartHash);
		if (_startName.empty())
		{
			ImGui::Text("DefaultStart     : (unknown) %u", _defaultStartHash);
		}
		else
		{
			ImGui::Text("DefaultStart     : %s", std::string(_startName).c_str());
		}

		ImGui::Separator();

		// ---- ノードエディタ ----
		// ImNodesのコンテキストをアセット側が持っているため、描画はアセットに任せる
		a_pAnimator->EditImGui(a_handle);
	}
}
