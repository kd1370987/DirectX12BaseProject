#pragma once

#include "../../../Engine/ECS/World/World.h"

//==========================================================================================
// HealthComponent
//
// 体力。敵・プレイヤーなど「殴られて減るもの」に付ける。
//
// ・maxHealth だけが設定値(保存され、インスペクタで編集できる)。
//   currentHealth は生成時に maxHealth で満たされるランタイム値で、表示のみ。
// ・減らすのは HealthSystem。0 になったらそのエンティティは自分で自分を消す。
// ・これを持っているものは ExplodeOnHitSystem の即死対象から外れる
//   (弾のように「当たった瞬間に消える」ものと住み分けるため)。
//==========================================================================================
struct HealthComponent
{
	float maxHealth = 100.0f;		// 最大体力 : 設定値
	float currentHealth = 0.0f;		// 現在体力 : ランタイム。HealthFixupSystem が maxHealth で満たす
};

template<>
struct Engine::ECS::ComponentTraits<HealthComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		HealthComponent& _comp = Engine::Editor::GetValue<HealthComponent>(a_pData);

		// 現在体力は保存しない。読み込み直したら満タンから始まる
		a_ar.Field("maxHealth", _comp.maxHealth);
	}

	static void Edit(CompEditContext& a_context)
	{
		HealthComponent& _comp = Engine::Editor::GetValue<HealthComponent>(a_context.pData);

		if (ImGui::DragFloat("MaxHealth", &_comp.maxHealth, 1.0f, 0.0f))
		{
			if (_comp.maxHealth < 0.0f) _comp.maxHealth = 0.0f;

			// 上限を下げたときに現在体力がはみ出したままにならないようにする
			if (_comp.currentHealth > _comp.maxHealth) _comp.currentHealth = _comp.maxHealth;
		}

		// 現在体力は表示のみ
		float _ratio = (_comp.maxHealth > 0.0f)
			? std::clamp(_comp.currentHealth / _comp.maxHealth, 0.0f, 1.0f)
			: 0.0f;

		char _label[32] = {};
		std::snprintf(_label, sizeof(_label), "%.0f / %.0f", _comp.currentHealth, _comp.maxHealth);
		ImGui::ProgressBar(_ratio, ImVec2(-FLT_MIN, 0.0f), _label);
		ImGui::SameLine();
		ImGui::Text("Current");
	}
};
