#pragma once

#include "Engine/Editor/Helper/EditorHelper.h"

// CollisionEvent がヒットしたときの反応を設定するコンポーネント。
//
// 出すエフェクトはここでは持たない。死亡時のエフェクトは DeathEffectComponent に
// 登録しておけば、着弾で消えるときも体力が尽きて消えるときも同じように出る。
struct ExplodeOnHitComponent
{
	bool destroySelf = true;		// 当たったら自分を消すか
};

template<>
struct Engine::ECS::ComponentTraits<ExplodeOnHitComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ExplodeOnHitComponent& _comp = Engine::Editor::GetValue<ExplodeOnHitComponent>(a_pData);
		a_ar.Field("destroySelf", _comp.destroySelf);
	}

	static void Edit(CompEditContext& a_context)
	{
		ExplodeOnHitComponent& _comp = Engine::Editor::GetValue<ExplodeOnHitComponent>(a_context.pData);

		ImGui::Checkbox("DestroySelf", &_comp.destroySelf);
		ImGui::TextDisabled("Effect is DeathEffectComponent.");
	}
};
