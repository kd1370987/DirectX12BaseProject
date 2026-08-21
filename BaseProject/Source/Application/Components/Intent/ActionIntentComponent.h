#pragma once

//==========================================================================================
// ActionIntentComponent
//
// 「持ち主が何を命じたか」だけを持つ。
// 左手の武器・右手の武器へは、押されているかどうかを伝えるだけで、
// 実際に撃てるか(連射間隔・バースト・オーバーヒート)は武器側が決める。
//
// 命令を武器へ渡すのは AttachmentDispatchSystem(武器が子の場合)と
// SelfWeaponTriggerSystem(本体が武器を兼ねる場合)。受け口は WeaponTriggerComponent。
//==========================================================================================
struct ActionIntentComponent
{
	// 左手の武器 : プレイヤーは左クリック
	bool isLeftWeaponShoot = false;

	// 右手の武器 : プレイヤーは右クリック
	bool isRightWeaponShoot = false;

	// ミサイル : 押している間ターゲットを溜め、離した瞬間に一斉射する。
	// 溜め/発射の面倒を見るのは MissileSalvoSystem
	bool isMissileHold = false;

	// どちらかの武器を撃とうとしているか。
	// アニメーションの Shoot パラメータのように「撃っているか」だけが要る所で使う
	bool IsAnyWeaponShoot() const { return isLeftWeaponShoot || isRightWeaponShoot; }
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
		auto _left = _comp.isLeftWeaponShoot;
		auto _right = _comp.isRightWeaponShoot;
		auto _missile = _comp.isMissileHold;
		ImGui::Checkbox("isLeftWeaponShoot", &_left);
		ImGui::Checkbox("isRightWeaponShoot", &_right);
		ImGui::Checkbox("isMissileHold", &_missile);
	}
};
