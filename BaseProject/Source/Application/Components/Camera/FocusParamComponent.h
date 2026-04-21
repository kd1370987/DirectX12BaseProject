#pragma once

struct FocusParamComponent
{
	float focusDistance		= 0.0f;  // 焦点距離
	float forcusRange		= 0.0f;   // 焦点範囲
	float forcusBackRange	= 1000.0f; // 焦点後ろ範囲

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const FocusParamComponent*>(a_ptr);
		a_json["forcusDistance"] = _comp->focusDistance;
		a_json["forcusRange"] = _comp->forcusRange;
		a_json["forcusBackRange"] = _comp->forcusBackRange;
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<FocusParamComponent*>(a_ptr);
		_comp->focusDistance = a_json.at("forcusDistance");
		_comp->forcusRange = a_json.at("forcusRange");
		_comp->forcusBackRange = a_json.at("forcusBackRange");
	}

	static void Edit(void* a_data)
	{
		FocusParamComponent& _comp = Engine::Editor::GetValue<FocusParamComponent>(a_data);
		ImGui::DragFloat("ForcusDistance", &_comp.focusDistance);
		ImGui::DragFloat("ForcusRange", &_comp.forcusRange);
		ImGui::DragFloat("ForcusBackRange", &_comp.forcusBackRange);

	}

};