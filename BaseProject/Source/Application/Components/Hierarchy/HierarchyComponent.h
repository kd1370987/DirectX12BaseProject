#pragma once

// 復元時はGUIDからの復元

struct HierarchyComponent
{
	// シリアライズ用
	Engine::GUID parentGUID = {};		// 親
	Engine::GUID firstChildGUID = {};	// 先頭のエンティティ
	Engine::GUID prevSiblingGUID = {};	// 自分より前のエンティティ
	Engine::GUID nextSiblingGUID = {};	// 自分の次に来るエンティティ

	// ランタイム用
	Engine::ECS::Entity parentID = Engine::ECS::Limits::INVALID_ENTITY;		// 親
	Engine::ECS::Entity firstChildID = Engine::ECS::Limits::INVALID_ENTITY;	// 先頭のエンティティ
	Engine::ECS::Entity prevSiblingID = Engine::ECS::Limits::INVALID_ENTITY;// 自分より前のエンティティ
	Engine::ECS::Entity nextSiblingID = Engine::ECS::Limits::INVALID_ENTITY;// 自分の次に来るエンティティ

	// 先頭から自分が何階層目なのか
	// ソート時につかう
	UINT depth = 0;

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const HierarchyComponent*>(a_ptr);
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<HierarchyComponent*>(a_ptr);
	}

	static void Edit(void* a_data)
	{
		HierarchyComponent& _comp = Engine::Editor::GetValue<HierarchyComponent>(a_data);
		ImGui::Text("ParentID : %d", _comp.parentID);
		ImGui::Text("FirstChildID : %d", _comp.firstChildID);
		ImGui::Text("PrevSiblingID : %d", _comp.prevSiblingID);
		ImGui::Text("NextSiblingID : %d", _comp.nextSiblingID);

		ImGui::Text("Depth",_comp.depth);
	}
};