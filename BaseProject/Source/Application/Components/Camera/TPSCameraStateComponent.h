#pragma once

// 注視点
struct TPSCameraStateComponent
{
	DirectX::XMFLOAT3 currentLookAt; // 現在カメラが見つめている実際の座標

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const TPSCameraStateComponent*>(a_ptr);
		Engine::JSONHelper::SetValue("currentLookAt",a_json, _comp->currentLookAt);
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		TPSCameraStateComponent& _comp = Engine::Editor::GetValue<TPSCameraStateComponent>(a_ptr);
		_comp.currentLookAt = Engine::JSONHelper::GetValue<DirectX::XMFLOAT3>("currentLookAt", a_json, {});
	}

	static void Edit(void* a_data)
	{
		TPSCameraStateComponent& _comp = Engine::Editor::GetValue<TPSCameraStateComponent>(a_data);
		ImGui::DragFloat3("currentLookAt", &_comp.currentLookAt.x, 0.0f);
	}
};