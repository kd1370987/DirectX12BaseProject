#pragma once

// 復元時はGUIDからの復元

struct HierarchyComponent
{
	// シリアライズ用
	Engine::GUID parentGUID = {};		// 親

	// ランタイム用
	Engine::ECS::Entity parentID = Engine::ECS::Limits::INVALID_ENTITY;		// 親

	// 先頭から自分が何階層目なのか
	// ソート時につかう
	UINT depth = 0;
};

template<>
struct Engine::ECS::ComponentTraits<HierarchyComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		HierarchyComponent& _comp = Engine::Editor::GetValue<HierarchyComponent>(a_pData);
		a_ar.Field("parentGUID", _comp.parentGUID);
		a_ar.Field("depth", _comp.depth);
	}

	static void Edit(CompEditContext& a_context)
	{
		HierarchyComponent& _comp = Engine::Editor::GetValue<HierarchyComponent>(a_context.pData);
		ImGui::Text("ParentGUID : %s", _comp.parentGUID.String().c_str());
		ImGui::Text("ParentID : %d", _comp.parentID);
		ImGui::Text("Depth : %d", _comp.depth);
	}
};