#pragma once

struct RayColliderComponent
{
	float length = 0.0f;
	DirectX::XMFLOAT3 dir = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT3 pos = { 0.0f,0.0f,0.0f };

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const RayColliderComponent*>(a_ptr);
		a_json["length"] = _comp->length;
		a_json["dir"] = { _comp->dir.x, _comp->dir.y, _comp->dir.z};
		a_json["pos"] = { _comp->pos.x,_comp->pos.y,_comp->pos.z };
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<RayColliderComponent*>(a_ptr);
		_comp->length = a_json.at("length");
		//_comp->dir = a_json["dir"].get<>();
		//_comp->pos = a_json.at("pos");
	}

	static void Edit(void* a_data)
	{
		using namespace Engine;
		RayColliderComponent& _comp = Engine::Editor::GetValue<RayColliderComponent>(a_data);
		ImGui::DragFloat("Length", &_comp.length, 0.1f);
		ImGui::DragFloat3("Dir", &_comp.dir.x, 0.1f);
		ImGui::DragFloat3("Pos", &_comp.pos.x, 0.1f);
	}
};