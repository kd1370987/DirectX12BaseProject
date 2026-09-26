#pragma once

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Editor/Helper/EditorField.h"

#include "Application/Utility/PrefabSpawnHelper.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Persistence/GUIDComponent.h"

//==========================================================================================
// BoidLeaderComponent
// 
// ボイド、小隊長を率いる今のところマーカーだが状態を持たせる予定
//
// ・付けるのは SwarmBossController。プレハブに入れ忘れていても生成時に足される。
// ・向き(LookAngleComponent)は SwarmLookSystem が進んでいる向きへ turnSpeedDeg で寄せる。
//   小隊長はこの向きの後ろを追いかけるので、列全体の進路はここが決めることになる。
//==========================================================================================
struct BoidLeaderComponent
{
	float turnSpeedDeg = 270.0f;	// 進んでいる向きへ向き直る速さ(度/秒。保存される)
};

template<>
struct Engine::ECS::ComponentTraits<BoidLeaderComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		BoidLeaderComponent& _comp = Engine::Editor::GetValue<BoidLeaderComponent>(a_pData);
		a_ar.Field("turnSpeedDeg", _comp.turnSpeedDeg);
	}

	static void Edit(CompEditContext& a_context)
	{
		BoidLeaderComponent& _comp = Engine::Editor::GetValue<BoidLeaderComponent>(a_context.pData);
		Engine::Editor::Field("TurnSpeedDeg", _comp.turnSpeedDeg, 1.0f, 0.0f);
	}
};
