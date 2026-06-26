#pragma once

struct LocalTransformComponent
{
	DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 quat = { 0.0f, 0.0f, 0.0f,1.0f };
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	mutable bool isDirty = true;

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const LocalTransformComponent*>(a_ptr);
		a_json["pos"] = { _comp->pos.x,_comp->pos.y,_comp->pos.z };
		a_json["quat"] = { _comp->quat.x,_comp->quat.y,_comp->quat.z ,_comp->quat.w };
		a_json["scale"] = { _comp->scale.x,_comp->scale.y,_comp->scale.z };

	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<LocalTransformComponent*>(a_ptr);
		using namespace Engine;
		auto _pos = a_json.at("pos");
		_comp->pos.x = _pos[0].get<float>();
		_comp->pos.y = _pos[1].get<float>();
		_comp->pos.z = _pos[2].get<float>();

		auto _quat = a_json.at("quat");
		_comp->quat.x = _quat[0].get<float>();
		_comp->quat.y = _quat[1].get<float>();
		_comp->quat.z = _quat[2].get<float>();
		_comp->quat.w = _quat[3].get<float>();

		auto _scale = a_json.at("scale");
		_comp->scale.x = _scale[0].get<float>();
		_comp->scale.y = _scale[1].get<float>();
		_comp->scale.z = _scale[2].get<float>();

		// 最初は変更
		_comp->isDirty = true;
	}

	static void Edit(void* a_data)
	{
		LocalTransformComponent& _comp = Engine::Editor::GetValue<LocalTransformComponent>(a_data);
		ImGui::DragFloat3("Position", &_comp.pos.x);
		ImGui::DragFloat3("Quaternion", &_comp.quat.x);
		ImGui::DragFloat3("Scale", &_comp.scale.x);
	}
};