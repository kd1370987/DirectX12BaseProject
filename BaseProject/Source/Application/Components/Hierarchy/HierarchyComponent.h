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

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const HierarchyComponent*>(a_ptr);
		a_json["Hierarchy_parentGUID"] = _comp->parentGUID.String();
		a_json["Hierarchy_Depth"] = _comp->depth;

	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<HierarchyComponent*>(a_ptr);
		_comp->parentGUID.FromString(a_json["Hierarchy_parentGUID"].get<std::string>());
		UINT _defaultDepth = 0;
		_comp->depth = Engine::JSONHelper::GetValue("Hierarchy_Depth", a_json, _defaultDepth);
	}

	static void Edit(void* a_data)
	{
		HierarchyComponent& _comp = Engine::Editor::GetValue<HierarchyComponent>(a_data);
		ImGui::Text("ParentGUID : %s", _comp.parentGUID.String().c_str());
		ImGui::Text("ParentID : %d", _comp.parentID);

		ImGui::Text("Depth : %d",_comp.depth);
	}
};