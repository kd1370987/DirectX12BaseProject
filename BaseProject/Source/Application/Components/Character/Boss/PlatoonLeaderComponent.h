#pragma once

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Editor/Helper/EditorHelper.h"

#include "Application/Utility/PrefabSpawnHelper.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Persistence/GUIDComponent.h"

//==========================================================================================
// リーダーに追従する小隊長
//==========================================================================================
struct PlatoonLeaderComponent
{
	Engine::ECS::Entity preLeader = Engine::ECS::Limits::INVALID_ENTITY;	// 一つ前の小隊長
	float distance = 0.0f;			// 前の小隊長からの間隔
};

template<>
struct Engine::ECS::ComponentTraits<PlatoonLeaderComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		PlatoonLeaderComponent& _comp = Engine::Editor::GetValue<PlatoonLeaderComponent>(a_pData);

	}

	static void Edit(CompEditContext& a_context)
	{
		PlatoonLeaderComponent& _comp = Engine::Editor::GetValue<PlatoonLeaderComponent>(a_context.pData);

	}
};
