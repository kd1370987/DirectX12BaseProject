#pragma once
#include "../IPanel.h"

namespace Engine::GameObject
{
	class BaseObject;
	class GameObjectManager;
}

namespace Engine::Editor
{
	/// <summary>
	/// GameObjectManager に登録されている ECS外オブジェクトのヒエラルキーウィンドウ。
	/// 選択したオブジェクトはインスペクター(Gameモード)で編集される。
	/// </summary>
	/// <remarks>
	/// ドラッグ&ドロップで親子関係を組める。
	/// ただし効くのは**この一覧の並びだけ**で、座標も表示状態も親から伝わらない。
	/// オブジェクトが増えたときに「これは誰のものか」を見て分かるようにするためのもの。
	/// </remarks>
	class GameObjectHierarchyPanel : public IPanel
	{
	public:
		~GameObjectHierarchyPanel() override = default;

		const char* GetName() const override { return "GameObjectHierarchy"; }
		void OnDrawImGui(EditorContext& a_editContext) override;

	private:

		// 親GUID → 子の一覧
		using ChildMap = std::unordered_map<Engine::GUID, std::vector<GameObject::BaseObject*>>;

		// 1件ぶんのノードを出す(子があれば折りたたみにする)
		void DrawObjectNode(
			EditorContext& a_editContext,
			GameObject::GameObjectManager* a_pManager,
			GameObject::BaseObject* a_pObject,
			const ChildMap& a_childMap,
			bool& a_inoutIsSelectedAlive);

		// 直前の項目がクリックされていたら選択する
		void HandleSelect(
			EditorContext& a_editContext,
			GameObject::BaseObject* a_pObject,
			bool& a_inoutIsSelectedAlive);

		/// <summary>
		/// 直前の項目の右クリックメニュー
		/// </summary>
		/// <remarks>
		/// ポップアップは「直前の項目」の状態を消費するので、
		/// 選択判定とドラッグ&ドロップより後に呼ぶこと
		/// </remarks>
		void HandleContextMenu(
			EditorContext& a_editContext,
			GameObject::BaseObject* a_pObject);

		// ドラッグ&ドロップで親を付け替える
		void HandleDragAndDrop(
			GameObject::GameObjectManager* a_pManager,
			GameObject::BaseObject* a_pObject);

		/// <summary>
		/// a_pParent をたどっていくと a_pChild に行き着くか
		/// </summary>
		/// <remarks>輪ができると一覧の描画が無限に潜るので、付け替える前に必ず見る</remarks>
		static bool IsDescendantOf(
			GameObject::GameObjectManager* a_pManager,
			const GameObject::BaseObject* a_pParent,
			const GameObject::BaseObject* a_pChild);
	};
}
