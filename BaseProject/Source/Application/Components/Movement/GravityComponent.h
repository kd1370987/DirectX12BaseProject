#pragma once

namespace App::Component
{
	struct GravityComponent
	{
		float scale = -1.0f;
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::GravityComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::GravityComponent& _comp = Engine::EditorField::GetValue<App::Component::GravityComponent>(a_pData);
		a_ar.Field("scale", _comp.scale);
	}

	static void Edit(CompEditContext& a_context)
	{
		App::Component::GravityComponent& _comp = Engine::EditorField::GetValue<App::Component::GravityComponent>(a_context.pData);
		Engine::EditorField::Field("GravityScale", _comp.scale, 0.1f);
	}
};