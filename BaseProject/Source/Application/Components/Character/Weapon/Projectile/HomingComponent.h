#pragma once
/// <summary>
/// 投射物が特定の対象を追跡する場合に付与される
/// </summary>
struct HomingComponent
{
	Engine::ECS::Entity targetEntity = Engine::ECS::Limits::INVALID_ENTITY;
	float turnSpeed = 0.0f;				// 旋回スピード
	float searchRange = 0.0f;			// ターゲットを探す最大距離
};

template<>
struct Engine::ECS::ComponentTraits<HomingComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		HomingComponent& _comp = Engine::Editor::GetValue<HomingComponent>(a_pData);
		a_ar.Field("turnSpeed",_comp.turnSpeed);
		a_ar.Field("searchRange",_comp.searchRange);
	}

	static void Edit(CompEditContext& a_context)
	{
		HomingComponent& _comp = Engine::Editor::GetValue<HomingComponent>(a_context.pData);
		ImGui::DragFloat("turnSpeed", &_comp.turnSpeed, 0.1f);
		ImGui::DragFloat("searchRange", &_comp.searchRange, 0.1f);
	}
};
