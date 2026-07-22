#pragma once

// 注視点
struct TPSCameraStateComponent
{
	DirectX::XMFLOAT3 currentLookAt = { 0.0f, 0.0f, 0.0f }; // 現在カメラが見つめている実際の座標
	DirectX::XMFLOAT4 currentOrbit  = { 0.0f, 0.0f, 0.0f, 1.0f }; // 現在のオービット回転(Slerp補間用)
};

template<>
struct Engine::ECS::ComponentTraits<TPSCameraStateComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		TPSCameraStateComponent& _comp = Engine::Editor::GetValue<TPSCameraStateComponent>(a_pData);
		a_ar.Field("currentLookAt", _comp.currentLookAt);
		a_ar.Field("currentOrbit", _comp.currentOrbit);
	}

	static void Edit(CompEditContext& a_context)
	{
		TPSCameraStateComponent& _comp = Engine::Editor::GetValue<TPSCameraStateComponent>(a_context.pData);
		ImGui::DragFloat3("currentLookAt", &_comp.currentLookAt.x, 0.0f);
	}
};