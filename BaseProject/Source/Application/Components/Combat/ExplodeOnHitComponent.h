#pragma once

#include "Engine/EditorField/EditorField.h"

namespace App::Component
{
	// CollisionEvent がヒットしたときの反応を設定するコンポーネント。
	//
	// 出すエフェクトはここでは持たない。死亡時のエフェクトは EffectEventsComponent の OnDeath に
	// 登録しておけば、着弾で消えるときも体力が尽きて消えるときも同じように出る。
	struct ExplodeOnHitComponent
	{
		bool destroySelf = true;		// 当たったら自分を消すか
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::ExplodeOnHitComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::ExplodeOnHitComponent& _comp = Engine::EditorField::RefValue<App::Component::ExplodeOnHitComponent>(a_pData);
		a_ar.Field("destroySelf", _comp.destroySelf);
	}

	static void Edit(CompEditContext& a_context)
	{
		App::Component::ExplodeOnHitComponent& _comp = Engine::EditorField::RefValue<App::Component::ExplodeOnHitComponent>(a_context.pData);

		Engine::EditorField::Field("DestroySelf", _comp.destroySelf);
		Engine::EditorField::Tooltip("Effect is EffectEventsComponent (OnDeath).");
	}
};
