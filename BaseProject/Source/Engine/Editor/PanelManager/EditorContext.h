#pragma once
namespace Engine::Editor
{
	/// <summary>
	/// インスペクターのモード
	/// </summary>
	enum class EInspectorType
	{
		None,
		Entity,
		Asset,
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
	};
}