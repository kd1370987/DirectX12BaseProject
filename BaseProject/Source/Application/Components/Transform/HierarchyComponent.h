#pragma once

namespace App::Component
{
	// 復元時はGUIDからの復元
	struct HierarchyComponent
	{
		// シリアライズ用
		Core::GUID parentGUID = {};		// 親

		// ランタイム用
		Engine::ECS::Entity parentID = Engine::ECS::Limits::INVALID_ENTITY;		// 親

		// 先頭から自分が何階層目なのか
		// ソート時につかう
		UINT depth = 0;
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::HierarchyComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::HierarchyComponent& _comp = Engine::EditorField::GetValue<App::Component::HierarchyComponent>(a_pData);
		a_ar.Field("parentGUID", _comp.parentGUID);
		a_ar.Field("depth", _comp.depth);
	}

	static void Edit(CompEditContext& a_context)
	{
		App::Component::HierarchyComponent& _comp = Engine::EditorField::GetValue<App::Component::HierarchyComponent>(a_context.pData);
		Engine::EditorField::Value("ParentGUID", "%s", _comp.parentGUID.String().c_str());
		Engine::EditorField::Value("ParentID", "%d", _comp.parentID);
		Engine::EditorField::Value("Depth", "%d", _comp.depth);
	}
};