#pragma once

/// <summary>
/// カメラ本体ではなく、カメラがフォーカスする対象につけるコンポーネント
/// </summary>
struct CameraForcusTargetComponent
{
	DirectX::XMFLOAT3 offsetPos;
};

template<>
struct Engine::ECS::ComponentTraits<CameraForcusTargetComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		CameraForcusTargetComponent& _comp = Engine::Editor::GetValue<CameraForcusTargetComponent>(a_pData);
		a_ar.Field("offsetPos", _comp.offsetPos);
	}

	static void Edit(CompEditContext& a_context)
	{
		CameraForcusTargetComponent& _comp = Engine::Editor::GetValue<CameraForcusTargetComponent>(a_context.pData);
		ImGui::DragFloat3("OffsetPos", &_comp.offsetPos.x,0.01f);
	}
};