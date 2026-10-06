#pragma once

namespace App::Component
{
	// 復元時はGUIDからの復元
	struct MoveIntentComponent
	{
		Math::Vector3 value;
		float jumpPow = 0.0f;
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::MoveIntentComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::MoveIntentComponent& _comp = Engine::EditorField::RefValue<App::Component::MoveIntentComponent>(a_pData);
		a_ar.Field("jumpPow", _comp.jumpPow);
	}

	static void Edit(CompEditContext& a_context)
	{
		App::Component::MoveIntentComponent& _comp = Engine::EditorField::RefValue<App::Component::MoveIntentComponent>(a_context.pData);
		Engine::EditorField::Header("MoveIntent");
		Engine::EditorField::Value("x", "%f", _comp.value.x);
		Engine::EditorField::Value("y", "%f", _comp.value.y);
		Engine::EditorField::Value("z", "%f", _comp.value.z);

		Engine::EditorField::Line();

		Engine::EditorField::Field("JumpPow", _comp.jumpPow, 0.01f, 0);
	}
};