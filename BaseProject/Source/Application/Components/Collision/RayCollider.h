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
		using namespace Engine;
		auto* _comp = static_cast<RayColliderComponent*>(a_ptr);
		_comp->length = a_json["length"].get<float>();
		
		//auto _dir = a_json.at("dir");
		_comp->dir = JSONHelper::GetVec3("dir", a_json, { 0,0,0 });
		//_comp->dir.x = _dir[0].get<float>();
		//_comp->dir.y = _dir[1].get<float>();
		//_comp->dir.z = _dir[2].get<float>();

		//auto _pos = a_json.at("pos");
		_comp->pos = JSONHelper::GetVec3("pos", a_json, { 0,0,0 });
		//_comp->pos.x = _pos[0].get<float>();
		//_comp->pos.y = _pos[1].get<float>();
		//_comp->pos.z = _pos[2].get<float>();
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