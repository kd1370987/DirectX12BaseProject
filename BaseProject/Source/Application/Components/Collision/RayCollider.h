#pragma once

// 地面判定用のレイ（足元原点から真下へ撃つ前提）
// 発射点は「足元 + stepUp」、長さは「stepUp + snapDown」の単一プローブ。
struct RayColliderComponent
{
	float stepUp   = 0.3f;	// 登れる段差＝発射点を足元より上げる量
	float snapDown = 0.4f;	// 足元より下で地面に吸着する距離
};

template<>
struct Engine::ECS::ComponentTraits<RayColliderComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		RayColliderComponent& _comp = Engine::Editor::GetValue<RayColliderComponent>(a_pData);
		a_ar.Field("stepUp", _comp.stepUp);
		a_ar.Field("snapDown", _comp.snapDown);
	}

	static void Edit(CompEditContext& a_context)
	{
		RayColliderComponent& _comp = Engine::Editor::GetValue<RayColliderComponent>(a_context.pData);
		ImGui::DragFloat("StepUp (登れる段差)", &_comp.stepUp, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("SnapDown (吸着距離)", &_comp.snapDown, 0.01f, 0.0f, 10.0f);
	}
};
