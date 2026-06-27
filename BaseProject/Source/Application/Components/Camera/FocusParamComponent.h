#pragma once

struct FocusParamComponent
{
	float focusDistance		= 0.0f;  // 焦点距離
	float focusRange		= 0.0f;   // 焦点範囲
	float focusBackRange	= 1000.0f; // 焦点後ろ範囲

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const FocusParamComponent*>(a_ptr);
		a_json["forcusDistance"] = _comp->focusDistance;
		a_json["forcusRange"] = _comp->focusRange;
		a_json["forcusBackRange"] = _comp->focusBackRange;
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<FocusParamComponent*>(a_ptr);
		using namespace Engine;
		_comp->focusDistance = JSONHelper::GetValue<float>("forcusDistance", a_json, 0.0f);
		_comp->focusRange = JSONHelper::GetValue<float>("forcusRange", a_json, 0.0f);
		_comp->focusBackRange = JSONHelper::GetValue<float>("forcusBackRange", a_json, 0.0f);
	}

	static void Edit(void* a_data)
	{
		FocusParamComponent& _comp = Engine::Editor::GetValue<FocusParamComponent>(a_data);
		ImGui::DragFloat("ForcusDistance", &_comp.focusDistance);
		ImGui::DragFloat("ForcusRange", &_comp.focusRange);
		ImGui::DragFloat("ForcusBackRange", &_comp.focusBackRange);

	}

};

template<>
struct Engine::ECS::ComponentTraits<FocusParamComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {
		auto* _comp = static_cast<FocusParamComponent*>(a_pData);
		a_ar.Field("focusDistance", _comp->focusDistance);
		a_ar.Field("focusRange", _comp->focusRange);
		a_ar.Field("focusBackRange", _comp->focusBackRange);
	}

	static void Edit(void* a_pData) {
		auto* _comp = static_cast<FocusParamComponent*>(a_pData);
		ImGui::DragFloat("ForcusDistance", &_comp->focusDistance);
		ImGui::DragFloat("ForcusRange", &_comp->focusRange);
		ImGui::DragFloat("ForcusBackRange", &_comp->focusBackRange);
	}
};