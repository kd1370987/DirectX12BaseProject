#pragma once

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Editor/Helper/EditorHelper.h"

#include "Application/Utility/PrefabSpawnHelper.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Persistence/GUIDComponent.h"

//==========================================================================================
// BoidLeaderComponent
// 
// ボイド、小隊長を率いる今のところマーカーだが状態を持たせる予定
//
// ・付けるのは SwarmBossController。プレハブに入れ忘れていても生成時に足される。
//==========================================================================================
struct BoidLeaderComponent
{

};

template<>
struct Engine::ECS::ComponentTraits<BoidLeaderComponent>
{

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
	
	}

	static void Edit(CompEditContext& a_context)
	{
	
	}

};
