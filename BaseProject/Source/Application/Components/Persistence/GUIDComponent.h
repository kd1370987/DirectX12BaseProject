#pragma once
struct GUIDComponent
{
	Engine::GUID guid = {};
};

template<>
struct Engine::ECS::ComponentTraits<GUIDComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		GUIDComponent& _comp = Engine::Editor::GetValue<GUIDComponent>(a_pData);
		a_ar.Field("guid", _comp.guid);
	}

	static void Edit(void* a_pData)
	{
		GUIDComponent& _comp = Engine::Editor::GetValue<GUIDComponent>(a_pData);
		ImGui::Text("%s", _comp.guid.String().c_str());
	}
};