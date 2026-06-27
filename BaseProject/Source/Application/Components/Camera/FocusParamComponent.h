#pragma once

struct FocusParamComponent
{
	float focusDistance		= 0.0f;  // 焦点距離
	float focusRange		= 0.0f;   // 焦点範囲
	float focusBackRange	= 1000.0f; // 焦点後ろ範囲
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