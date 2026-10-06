#pragma once
#include "Engine/Scene/SceneManager/SceneManager.h"
#include "Engine/ECS/World/World.h"
#include "Application/Components/Core/GUIDComponent.h"

namespace App::Component
{
	struct FollowTargetComponent
	{
		Core::GUID targetGUID = {};
		Engine::ECS::Entity target = Engine::ECS::Limits::INVALID_ENTITY;
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::FollowTargetComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {
		auto* _comp = static_cast<App::Component::FollowTargetComponent*>(a_pData);
		a_ar.Field("targetGUID", _comp->targetGUID);
	}

	static void Edit(CompEditContext& a_context) {
		using namespace Engine;
		App::Component::FollowTargetComponent& _comp = Engine::EditorField::GetValue<App::Component::FollowTargetComponent>(a_context.pData);

		ECS::Entity _entity = _comp.target;

		Engine::EditorField::Field("TargetEntity", _entity);
		Engine::EditorField::Text("%s", _comp.targetGUID.String().c_str());

		// エンティティの変更がされたらGUIDを変更
		if (_entity != _comp.target)
		{
			auto* _pWorld = Engine::Scene::SceneManager::Instance().RefWorld();
			auto _typeID = _pWorld->GetCompTypeID<App::Component::GUIDComponent>();
			uint8_t* _data = _pWorld->NRefData(_entity, _typeID);
			App::Component::GUIDComponent& _targetGUIDComp = *reinterpret_cast<App::Component::GUIDComponent*>(_data);
			_comp.targetGUID = _targetGUIDComp.guid;
			_comp.target = _pWorld->GetEntity(_comp.targetGUID);
		}
	}
};