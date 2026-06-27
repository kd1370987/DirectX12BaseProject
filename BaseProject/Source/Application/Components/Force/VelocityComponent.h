#pragma once

struct VelocityComponent
{
	DirectX::XMFLOAT3 value = { 0.0f, 0.0f, 0.0f };

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const VelocityComponent*>(a_ptr);
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<VelocityComponent*>(a_ptr);
	}

	static void Edit(void* a_data)
	{
		VelocityComponent& _comp = Engine::Editor::GetValue<VelocityComponent>(a_data);
		ImGui::LabelText("","%.2f, %.2f , %.2f", _comp.value.x, _comp.value.y, _comp.value.z);
		if (ImGui::Button("Clear"))
		{
			_comp.value = {};
		}
	}
};

template<>
struct Engine::ECS::ComponentTraits<VelocityComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		VelocityComponent& _comp = Engine::Editor::GetValue<VelocityComponent>(a_pData);
	}

	static void Edit(void* a_pData)
	{
		VelocityComponent& _comp = Engine::Editor::GetValue<VelocityComponent>(a_pData);
		ImGui::LabelText("", "%.2f, %.2f , %.2f", _comp.value.x, _comp.value.y, _comp.value.z);
		if (ImGui::Button("Clear"))
		{
			_comp.value = {};
		}
	}
};
