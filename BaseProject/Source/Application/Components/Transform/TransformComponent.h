#pragma once

#include "../../../Engine/Editor/ImGui/ImGuiHelper/ImGuiHelper.h"
struct TransformComponent
{
	DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 quat = { 0.0f, 0.0f, 0.0f,1.0f };
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const TransformComponent*>(a_ptr);
		Engine::JSONHelper::SetValue("pos", a_json, _comp->pos);
		Engine::JSONHelper::SetValue("quat", a_json, _comp->quat);
		Engine::JSONHelper::SetValue("scale", a_json, _comp->scale);
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		using namespace Engine;
		auto* _comp = static_cast<TransformComponent*>(a_ptr);
		_comp->pos   = JSONHelper::GetValue("pos",a_json,DirectX::XMFLOAT3());
		_comp->quat  = JSONHelper::GetValue("quat", a_json, DirectX::XMFLOAT4());
		_comp->scale = JSONHelper::GetValue("scale", a_json, DirectX::XMFLOAT3());
	}

	static void Edit(void* a_data)
	{
		TransformComponent& _comp = Engine::Editor::GetValue<TransformComponent>(a_data);
		ImGui::DragFloat3("Position",&_comp.pos.x,0.1f);
		Engine::Editor::Helper::DragRotationDeg3FromQuaternion(_comp.quat);
		ImGui::DragFloat3("Scale",&_comp.scale.x,0.1f);
	}
};