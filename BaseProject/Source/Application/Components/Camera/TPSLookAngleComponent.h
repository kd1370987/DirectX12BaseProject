#pragma once

struct TPSLookAngleComponent
{
	float Pitch = 0.0f;
	float ClampPitch = 80.0f;

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const TPSLookAngleComponent*>(a_ptr);
		a_json["Pitch"] = _comp->Pitch;
		a_json["ClampPitch"] = _comp->ClampPitch;
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<TPSLookAngleComponent*>(a_ptr);
		using namespace Engine;
		_comp->Pitch = JSONHelper::GetValue<float>("Pitch", a_json, 0.0f);
		_comp->ClampPitch = JSONHelper::GetValue<float>("ClampPitch", a_json, 80.0f);
	}

	static void Edit(void* a_data)
	{
		TPSLookAngleComponent& _comp = Engine::Editor::GetValue<TPSLookAngleComponent>(a_data);
		ImGui::DragFloat("Pitch", &_comp.Pitch);
		ImGui::DragFloat("CrampPitch", &_comp.ClampPitch);
	}

};

template<>
struct Engine::ECS::ComponentTraits<TPSLookAngleComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		TPSLookAngleComponent& _comp = Engine::Editor::GetValue<TPSLookAngleComponent>(a_pData);
		a_ar.Field("Pitch", _comp.Pitch);
		a_ar.Field("ClampPitch", _comp.ClampPitch);
	}

	static void Edit(void* a_pData)
	{
		TPSLookAngleComponent& _comp = Engine::Editor::GetValue<TPSLookAngleComponent>(a_pData);
		ImGui::DragFloat("Pitch", &_comp.Pitch);
		ImGui::DragFloat("CrampPitch", &_comp.ClampPitch);
	}
};