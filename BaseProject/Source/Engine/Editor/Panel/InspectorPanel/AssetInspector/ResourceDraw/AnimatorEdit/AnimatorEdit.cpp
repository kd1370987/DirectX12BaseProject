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
		Engine::Editor::Value("Name", "%s", a_pAnimator->GetName().c_str());

		// 開始ステート名 : ハッシュから引けなければハッシュのまま表示
		UINT _defaultStartHash = a_pAnimator->GetDefaultStartHash();
		auto _startName = a_pAnimator->GetNodeName(_defaultStartHash);
		if (_startName.empty())
		{
			Engine::Editor::Value("DefaultStart", "(unknown) %u", _defaultStartHash);
		}
		else
		{
			Engine::Editor::Value("DefaultStart", "%s", std::string(_startName).c_str());
		}

		Engine::Editor::Line();

		// ---- ノードエディタ ----
		// ImNodesのコンテキストをアセット側が持っているため、描画はアセットに任せる
		if (a_editContext.pServices) a_pAnimator->EditImGui(a_handle, *a_editContext.pServices);
	}
}
