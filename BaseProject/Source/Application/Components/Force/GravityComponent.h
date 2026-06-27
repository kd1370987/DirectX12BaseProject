#pragma once

struct GravityComponent
{
	float scale = -1.0f;

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const GravityComponent*>(a_ptr);
		a_json["scale"] = _comp->scale;
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<GravityComponent*>(a_ptr);
		using namespace Engine;
		_comp->scale = JSONHelper::GetValue<float>("scale",a_json,-1.0f);
	}

	static void Edit(void* a_data)
	{
		GravityComponent& _comp = Engine::Editor::GetValue<GravityComponent>(a_data);
		ImGui::DragFloat("GravityScale", &_comp.scale, 0.1f);

	}
};

template<>
struct Engine::ECS::ComponentTraits<GravityComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		GravityComponent& _comp = Engine::Editor::GetValue<GravityComponent>(a_pData);
		a_ar.Field("scale", _comp.scale);
	}

	static void Edit(void* a_pData)
	{
		GravityComponent& _comp = Engine::Editor::GetValue<GravityComponent>(a_pData);
		ImGui::DragFloat("GravityScale", &_comp.scale, 0.1f);
	}
};