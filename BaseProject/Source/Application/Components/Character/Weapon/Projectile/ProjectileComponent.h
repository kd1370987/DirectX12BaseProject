#pragma once

/// <summary>
/// 武器から発射する投射物につくステート
/// </summary>
struct ProjectileComponent
{
	// 撃った本体。発射の瞬間に GunShootSystem が埋める(保存しない)。
	// 銃が子エンティティの場合でも、コライダーを持つ本体の方が入る。
	// HitDetectSystem がこの相手を判定から除外する(自分の弾に当たらないように)
	Engine::ECS::Entity shooterEntity = Engine::ECS::Limits::INVALID_ENTITY;
	float speed = 0.0f;			// スピード
	float damage = 0.0f;		// ヒット時ダメージ

	// 生存時間は LifeTimeComponent が持つ。時間で消したい弾にはそちらを付けること
};

template<>
struct Engine::ECS::ComponentTraits<ProjectileComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ProjectileComponent& _comp = Engine::Editor::GetValue<ProjectileComponent>(a_pData);
		a_ar.Field("speed", _comp.speed);
		a_ar.Field("damage", _comp.damage);
	}

	static void Edit(CompEditContext& a_context)
	{
		ProjectileComponent& _comp = Engine::Editor::GetValue<ProjectileComponent>(a_context.pData);
		ImGui::DragFloat("speed", &_comp.speed, 0.1f);
		ImGui::DragFloat("damage", &_comp.damage, 0.1f);

		// 発射元(表示のみ。発射時に埋まる)
		if (_comp.shooterEntity == Engine::ECS::Limits::INVALID_ENTITY)
		{
			ImGui::TextDisabled("Shooter : None");
		}
		else
		{
			ImGui::TextDisabled("Shooter : %llu", static_cast<unsigned long long>(_comp.shooterEntity));
		}
	}
};