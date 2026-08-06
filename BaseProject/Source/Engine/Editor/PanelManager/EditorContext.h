#pragma once

namespace Engine::GameObject
{
	class BaseObject;
}

namespace Engine::Editor
{
	class EditorCamera;
	class Profiler;

	/// <summary>
	/// インスペクターのモード
	/// </summary>
	enum class EInspectorType
	{
		None,
		Entity,
		Asset,
		Game,
	};

	/// <summary>
	/// パネル間でやり取りされるメモ帳
	/// </summary>
	struct EditorContext
	{
		float appWindowWidth = 0;
		float appWindowHeight = 0;

		// 現在のインスペクターモード
		EInspectorType eInspectorType = EInspectorType::None;

		// 選択中のアセット
		Resource::AssetProperty* pAssetProp = nullptr;

		// 選択中のエンティティ
		ECS::Entity entity = ECS::Limits::INVALID_ENTITY;

		// 選択中のECS外オブジェクト(GameObjectManager管理下)
		GameObject::BaseObject* pGameObject = nullptr;

		// エディターカメラポインタ
		EditorCamera* pEditorCamera = nullptr;

		// プロファイラポインタ : 計測はプロファイラ側が行い、パネルは結果を読むだけ
		Profiler* pProfiler = nullptr;
	};
}