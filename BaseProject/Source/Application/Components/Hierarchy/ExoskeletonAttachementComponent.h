#pragma once

#include "../../../Engine/Editor/ImGui/ImGuiHelper/ImGuiHelper.h"
#include "../../Scene/SceneManager.h"
#include "../../../Engine/ECS/World/World.h"
#include "../Persistence/GUIDComponent.h"
struct ExoskeletonAttachmentComponent
{
	// 追従するエンティティ
	Engine::GUID parentGUID = {};											// シリアライズ用
	Engine::ECS::Entity parentID = Engine::ECS::Limits::INVALID_ENTITY;		// ランタイム用

	// 追従するボーンのID
	UINT targetBoneID = 0;	//後でストリングハッシュにするかも

	// オフセット情報
	DirectX::XMFLOAT3 offsetPosition = { 0,0,0 };
	DirectX::XMFLOAT4 offsetRotation = { 0,0,0,1 };

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const ExoskeletonAttachmentComponent*>(a_ptr);
		a_json["parentGUID"] = _comp->parentGUID.String();
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<ExoskeletonAttachmentComponent*>(a_ptr);
		_comp->parentGUID.FromString(a_json["targetGUID"].get<std::string>());
	}

	static void Edit(void* a_data)
	{
		ExoskeletonAttachmentComponent& _comp = Engine::Editor::GetValue<ExoskeletonAttachmentComponent>(a_data);
		auto _entity = _comp.parentID;
		ImGui::Text("ParentGUID : %s",_comp.parentGUID.String());
		ImGui::InputScalar("ParentID", ImGuiDataType_U64, &_entity);
		ImGui::Text("TargetBoneID : %d", _comp.targetBoneID);

		// エンティティの変更がされたらGUIDを変更
		if (_entity != _comp.parentID)
		{
			auto* _pWorld = SceneManager::Instance().RefWorld();
			auto _typeID = _pWorld->GetCompTypeID<GUIDComponent>();
			uint8_t* _data = _pWorld->NRefData(_entity, _typeID);
			GUIDComponent& _targetGUIDComp = *reinterpret_cast<GUIDComponent*>(_data);
			_comp.parentGUID = _targetGUIDComp.guid;
			_comp.parentID = _pWorld->GetEntity(_comp.parentGUID);
		}
		
		ImGui::DragFloat3("OffsetPos",&_comp.offsetPosition.x,0.1f);
		Engine::Editor::Helper::DragRotationDeg3FromQuaternion(_comp.offsetRotation);


	}
};