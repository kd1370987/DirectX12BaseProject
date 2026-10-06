#pragma once

#include "Engine/EditorField/EditorField.h"

namespace App::Component
{
	//==========================================================================================
	// DefenseRatioComponent
	//
	// 受けたダメージに掛ける比率。1ならそのまま食らい、0ならダメージも0(無敵)。
	//
	// ・掛けるのは HealthSystem(そのフレームに受けたダメージの合計に掛ける)。
	//   持っていないものは 1 として扱う(今までどおり)。
	// ・ratio は保存される設定値。実行中に書き換える側もある
	//   (ワームボスは小隊長を整理している間だけ、体のボイドを 0 にして無敵にする)。
	// ・0 未満は 0 として扱う(殴られて回復はしない)。
	//==========================================================================================
	struct DefenseRatioComponent
	{
		float ratio = 1.0f;		// 受けたダメージに掛ける比率(1 : そのまま / 0 : 無敵)
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::DefenseRatioComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::DefenseRatioComponent& _comp = Engine::EditorField::RefValue<App::Component::DefenseRatioComponent>(a_pData);
		a_ar.Field("ratio", _comp.ratio);
	}

	static void Edit(CompEditContext& a_context)
	{
		App::Component::DefenseRatioComponent& _comp = Engine::EditorField::RefValue<App::Component::DefenseRatioComponent>(a_context.pData);
		Engine::EditorField::Field("Ratio", _comp.ratio, 0.01f, 0.0f);
		Engine::EditorField::Tooltip("Damage x ratio (1 : as is / 0 : invincible)");
	}
};
