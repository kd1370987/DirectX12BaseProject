#pragma once

/// <summary>
/// 武器から発射する投射物につくステート
/// </summary>
struct ProjectileComponent
{
	Engine::ECS::Entity shooterEntity = Engine::ECS::Limits::INVALID_ENTITY;
	float speed = 0.0f;			// スピード
	float lifeTime = 0.0f;		// 生存時間 : 秒
	float damage = 0.0f;		// ヒット時ダメージ
};

template<>
struct Engine::ECS::ComponentTraits<ProjectileComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ProjectileComponent& _comp = Engine::Editor::GetValue<ProjectileComponent>(a_pData);
		a_ar.Field("speed", _comp.speed);
		a_ar.Field("lifeTime", _comp.lifeTime);
		a_ar.Field("damage", _comp.damage);
	}

	static void Edit(CompEditContext& a_context)
	{
		ProjectileComponent& _comp = Engine::Editor::GetValue<ProjectileComponent>(a_context.pData);
		ImGui::DragFloat("speed", &_comp.speed, 0.1f);
		ImGui::DragFloat("lifeTime", &_comp.lifeTime, 0.1f);
		ImGui::DragFloat("damage", &_comp.damage, 0.1f);
	}
};