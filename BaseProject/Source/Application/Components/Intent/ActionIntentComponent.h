#pragma once
struct ActionIntentComponent
{
	bool isGunShoot = false;
	bool isAiming = false;

	// ミサイル : 押している間ターゲットを溜め、離した瞬間に一斉射する。
	// 溜め/発射の面倒を見るのは MissileSalvoSystem
	bool isMissileHold = false;
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
		auto _missile = _comp.isMissileHold;
		ImGui::Checkbox("isGunShoot",&_gun);
		ImGui::Checkbox("isAiming",&_aim);
		ImGui::Checkbox("isMissileHold",&_missile);
	}
};