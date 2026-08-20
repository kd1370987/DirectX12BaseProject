#pragma once

#include "../../../Engine/ECS/World/World.h"

//==========================================================================================
// HealthComponent
//
// 体力と「死んでからの後始末待ち」を持つ。敵・ボス・プレイヤーなど
// 「殴られて減るもの」に付ける。
//
// ・maxHealth / releaseDelay だけが設定値(保存され、インスペクタで編集できる)。
//   currentHealth と死亡状態は生成時に初期化されるランタイム値で、表示のみ。
// ・減らすのは HealthSystem。0 になったらその場では消さず「死亡状態」へ入る。
//   死亡状態のあいだ入力/AIは止まり、releaseDelay 秒たったら DeathStateSystem が
//   解放予約する。
//
//   即座に消さないのは、消えたエンティティからは何も引けないため。
//   死亡エフェクト(DeathEffectComponent)を出す DeathEffectSystem は
//   「死んだ本人のコンポーネント」を引くので、本人が生きているうちに
//   出し切れるだけの猶予がいる。演出中に死体が残るのは意図した挙動でもある。
//
// ・これを持っているものは ExplodeOnHitSystem の即死対象から外れる
//   (弾のように「当たった瞬間に消える」ものと住み分けるため)。
//==========================================================================================
struct HealthComponent
{
	float maxHealth = 100.0f;		// 最大体力 : 設定値
	float currentHealth = 0.0f;		// 現在体力 : ランタイム。HealthFixupSystem が maxHealth で満たす

	// ---- 死亡状態 ----
	// 死んでから実際に消えるまでの猶予。演出(死亡エフェクト・やられモーション)の尺に合わせる。
	// 0 にすると次のフレームで消えるので、エフェクトを出す猶予が無くなる点に注意
	float releaseDelay = 2.0f;		// 死亡してから解放予約するまでの秒数 : 設定値

	bool  isDead = false;			// 死亡状態か : ランタイム
	float deathTimer = 0.0f;		// 死亡してからの経過秒 : ランタイム
};

template<>
struct Engine::ECS::ComponentTraits<HealthComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		HealthComponent& _comp = Engine::Editor::GetValue<HealthComponent>(a_pData);

		// 現在体力と死亡状態は保存しない。読み込み直したら満タンの生存から始まる
		a_ar.Field("maxHealth", _comp.maxHealth);
		a_ar.Field("releaseDelay", _comp.releaseDelay);
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

		ImGui::Separator();

		if (ImGui::DragFloat("ReleaseDelay", &_comp.releaseDelay, 0.05f, 0.0f))
		{
			if (_comp.releaseDelay < 0.0f) _comp.releaseDelay = 0.0f;
		}
		ImGui::TextDisabled("(死亡してから消えるまでの秒数)");

		// 死亡状態は表示のみ
		if (_comp.isDead)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
				"Dead : %.2f / %.2f", _comp.deathTimer, _comp.releaseDelay);
		}
		else
		{
			ImGui::TextDisabled("Alive");
		}
	}
};
