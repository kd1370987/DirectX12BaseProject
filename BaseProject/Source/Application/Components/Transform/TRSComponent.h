#pragma once

struct TRSComponent
{
	DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 quat = { 0.0f, 0.0f, 0.0f,1.0f };
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const TRSComponent*>(a_ptr);
		a_json["pos"] = { _comp->pos.x,_comp->pos.y,_comp->pos.z };
		a_json["quat"] = { _comp->quat.x,_comp->quat.y,_comp->quat.z ,_comp->quat.w };
		a_json["scale"] = { _comp->scale.x,_comp->scale.y,_comp->scale.z };

	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<TRSComponent*>(a_ptr);
	}

	static void Edit(void* a_data)
	{
		TRSComponent& _comp = Engine::Editor::GetValue<TRSComponent>(a_data);
		ImGui::DragFloat3("Position",&_comp.pos.x);
		ImGui::DragFloat3("Quaternion",&_comp.quat.x);
		ImGui::DragFloat3("Scale",&_comp.scale.x);
	}
};