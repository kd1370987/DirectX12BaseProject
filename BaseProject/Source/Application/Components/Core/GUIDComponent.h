#pragma once
namespace App::Component
{
	struct GUIDComponent
	{
		Core::GUID guid = {};
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::GUIDComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::GUIDComponent& _comp = Engine::EditorField::GetValue<App::Component::GUIDComponent>(a_pData);
		a_ar.Field("guid", _comp.guid);
	}

	static void Edit(CompEditContext& a_context)
	{
		App::Component::GUIDComponent& _comp = Engine::EditorField::GetValue<App::Component::GUIDComponent>(a_context.pData);
		Engine::EditorField::Text("%s", _comp.guid.String().c_str());
	}
};