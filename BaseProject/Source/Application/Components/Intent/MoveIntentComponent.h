#pragma once

// 復元時はGUIDからの復元

struct MoveIntentComponent
{
	DirectX::XMFLOAT3 value;
	float jumpPow = 0.0f;


	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const MoveIntentComponent*>(a_ptr);
		a_json["jumpPow"] = _comp->jumpPow;
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<MoveIntentComponent*>(a_ptr);
		_comp->jumpPow = Engine::JSONHelper::GetValue<float>("jumpPow", a_json, 5.0f);
	}

	static void Edit(void* a_data)
	{
		MoveIntentComponent& _comp = Engine::Editor::GetValue<MoveIntentComponent>(a_data);
		ImGui::Text("MoveIntent");
		ImGui::Text("x : %f", _comp.value.x);
		ImGui::Text("y : %f", _comp.value.y);
		ImGui::Text("z : %f", _comp.value.z);

		ImGui::Separator();

		ImGui::DragFloat("JumpPow", &_comp.jumpPow, 0.01f, 0);
	}
};

template<>
struct Engine::ECS::ComponentTraits<MoveIntentComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		MoveIntentComponent& _comp = Engine::Editor::GetValue<MoveIntentComponent>(a_pData);
		a_ar.Field("jumpPow", _comp.jumpPow);
	}

	static void Edit(void* a_pData)
	{
		MoveIntentComponent& _comp = Engine::Editor::GetValue<MoveIntentComponent>(a_pData);
		ImGui::Text("MoveIntent");
		ImGui::Text("x : %f", _comp.value.x);
		ImGui::Text("y : %f", _comp.value.y);
		ImGui::Text("z : %f", _comp.value.z);

		ImGui::Separator();

		ImGui::DragFloat("JumpPow", &_comp.jumpPow, 0.01f, 0);
	}
};