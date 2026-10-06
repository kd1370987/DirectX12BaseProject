#pragma once
namespace App::Component
{
	struct NameComponent
	{
		char name[64] = "Unknown";
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::NameComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::NameComponent& _comp = Engine::EditorField::GetValue<App::Component::NameComponent>(a_pData);
		a_ar.Field("name", _comp.name);
	}

	static void Edit(CompEditContext& a_context)
	{
		App::Component::NameComponent& _comp = Engine::EditorField::GetValue<App::Component::NameComponent>(a_context.pData);
		Engine::EditorField::Field("Name", _comp.name, 64);
	}
};