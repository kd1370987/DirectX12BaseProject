#pragma once
#include "../IPanel.h"

namespace Engine::Editor
{
	/// <summary>
	/// GameObjectManager に登録されている ECS外オブジェクトのヒエラルキーウィンドウ。
	/// 選択したオブジェクトはインスペクター(Gameモード)で編集される。
	/// </summary>
	class GameObjectHierarchyPanel : public IPanel
	{
	public:
		~GameObjectHierarchyPanel() override = default;

		const char* GetName() const override { return "GameObjectHierarchy"; }
		void OnDrawImGui(EditorContext& a_editContext) override;
	};
}
