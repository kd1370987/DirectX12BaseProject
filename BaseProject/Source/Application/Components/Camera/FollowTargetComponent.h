#pragma once
#include "Engine/Scene/SceneManager/SceneManager.h"
#include "../../../Engine/ECS/World/World.h"
#include "../Persistence/GUIDComponent.h"

struct FollowTargetComponent
{
	Engine::GUID targetGUID = {};
	Engine::ECS::Entity target = Engine::ECS::Limits::INVALID_ENTITY;

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const FollowTargetComponent*>(a_ptr);
		a_json["targetGUID"] = _comp->targetGUID.String();
		
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<FollowTargetComponent*>(a_ptr);
		_comp->targetGUID.FromString(a_json["targetGUID"].get<std::string>());

	}

	static void Edit(void* a_data)
	{
		using namespace Engine;
		FollowTargetComponent& _comp = Engine::Editor::GetValue<FollowTargetComponent>(a_data);
		
		ECS::Entity _entity = _comp.target;

		ImGui::InputScalar("TargetEntity", ImGuiDataType_U64, &_entity);
		ImGui::Text("%s",_comp.targetGUID.String().c_str());

		// エンティティの変更がされたらGUIDを変更
		if (_entity != _comp.target)
		{
			auto* _pWorld = Engine::Scene::SceneManager::Instance().RefWorld();
			auto _typeID = _pWorld->GetCompTypeID<GUIDComponent>();
			uint8_t* _data = _pWorld->NRefData(_entity,_typeID);
			GUIDComponent& _targetGUIDComp = *reinterpret_cast<GUIDComponent*>(_data);
			_comp.targetGUID = _targetGUIDComp.guid;
			_comp.target = _pWorld->GetEntity(_comp.targetGUID);
		}
	}
};

template<>
struct Engine::ECS::ComponentTraits<FollowTargetComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {
		auto* _comp = static_cast<FollowTargetComponent*>(a_pData);
		a_ar.Field("targetGUID", _comp->targetGUID);
	}

	static void Edit(void* a_pData) {
		using namespace Engine;
		FollowTargetComponent& _comp = Engine::Editor::GetValue<FollowTargetComponent>(a_pData);

		ECS::Entity _entity = _comp.target;

		ImGui::InputScalar("TargetEntity", ImGuiDataType_U64, &_entity);
		ImGui::Text("%s", _comp.targetGUID.String().c_str());

		// エンティティの変更がされたらGUIDを変更
		if (_entity != _comp.target)
		{
			auto* _pWorld = Engine::Scene::SceneManager::Instance().RefWorld();
			auto _typeID = _pWorld->GetCompTypeID<GUIDComponent>();
			uint8_t* _data = _pWorld->NRefData(_entity, _typeID);
			GUIDComponent& _targetGUIDComp = *reinterpret_cast<GUIDComponent*>(_data);
			_comp.targetGUID = _targetGUIDComp.guid;
			_comp.target = _pWorld->GetEntity(_comp.targetGUID);
		}
	}
};