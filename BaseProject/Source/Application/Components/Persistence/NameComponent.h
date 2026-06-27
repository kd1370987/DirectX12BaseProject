#pragma once
struct NameComponent
{
	char name[64] = "Unknown";
};

template<>
struct Engine::ECS::ComponentTraits<NameComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		NameComponent& _comp = Engine::Editor::GetValue<NameComponent>(a_pData);
		a_ar.Field("name", _comp.name);
	}

	static void Edit(void* a_pData)
	{
		NameComponent& _comp = Engine::Editor::GetValue<NameComponent>(a_pData);
		ImGui::InputText("##Name", _comp.name, 64);
		ImGui::SameLine();
		ImGui::Text("Name");
	}
};