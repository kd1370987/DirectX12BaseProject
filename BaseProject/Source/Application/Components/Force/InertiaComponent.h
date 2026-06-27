#pragma once

// 慣性コンポーネント

struct InertiaComponent
{
	float value = 0.0f;

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const InertiaComponent*>(a_ptr);
		a_json["Inertia"] = _comp->value;
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<InertiaComponent*>(a_ptr);
		using namespace Engine;
		_comp->value = JSONHelper::GetValue<float>("Inertia",a_json,0.0f);
	}

	static void Edit(void* a_data)
	{
		InertiaComponent& _comp = Engine::Editor::GetValue<InertiaComponent>(a_data);
		ImGui::DragFloat("Inertia", &_comp.value, 0.1f);

	}
};

template<>
struct Engine::ECS::ComponentTraits<InertiaComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		InertiaComponent& _comp = Engine::Editor::GetValue<InertiaComponent>(a_pData);
		a_ar.Field("Inertia", _comp.value);
	}

	static void Edit(void* a_pData)
	{
		InertiaComponent& _comp = Engine::Editor::GetValue<InertiaComponent>(a_pData);
		ImGui::DragFloat("Inertia", &_comp.value, 0.1f);
	}
};