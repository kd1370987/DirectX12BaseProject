#pragma once

// 注視点
struct TPSCameraStateComponent
{
	DirectX::XMFLOAT3 currentLookAt; // 現在カメラが見つめている実際の座標
};

template<>
struct Engine::ECS::ComponentTraits<TPSCameraStateComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		TPSCameraStateComponent& _comp = Engine::Editor::GetValue<TPSCameraStateComponent>(a_pData);
		a_ar.Field("currentLookAt", _comp.currentLookAt);
	}

	static void Edit(void* a_pData)
	{
		TPSCameraStateComponent& _comp = Engine::Editor::GetValue<TPSCameraStateComponent>(a_pData);
		ImGui::DragFloat3("currentLookAt", &_comp.currentLookAt.x, 0.0f);
	}
};