#pragma once
struct ActionIntentComponent
{
	bool isGunShoot = false;
	bool isAiming = false;
};

template<>
struct Engine::ECS::ComponentTraits<ActionIntentComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ActionIntentComponent& _comp = Engine::Editor::GetValue<ActionIntentComponent>(a_pData);
	}

	static void Edit(CompEditContext& a_context)
	{
		ActionIntentComponent& _comp = Engine::Editor::GetValue<ActionIntentComponent>(a_context.pData);
		auto _gun = _comp.isGunShoot;
		auto _aim = _comp.isAiming;
		ImGui::Checkbox("isGunShoot",&_gun);
		ImGui::Checkbox("isAiming",&_aim);
	}
};