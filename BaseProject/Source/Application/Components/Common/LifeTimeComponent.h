#pragma once

//==========================================================================================
// LifeTimeComponent
//
// 「時間で勝手に消えるもの」に付ける寿命。弾・エフェクトなど種類を問わない。
// 減らして消すのは LifeTimeSystem。
//
// ・value はそのまま残り時間として減らしていく。プレハブに書いてある値は
//   実体化のたびにコピーされるので、カウントダウンしても設計値は壊れない。
//   インスペクターに出るのは「残り何秒か」になる。
// ・負の値は無期限として扱う。0 まで減ったフレームで必ず消えるので、
//   生きているエンティティの値が負のまま残ることはなく、初期値の負値と区別がつく
//   (消すタイミングを別のシステムで決めたいとき用の逃げ道)。
//==========================================================================================
struct LifeTimeComponent
{
	float value = 1.0f;		// 残り生存時間(秒)。負なら無期限
};

template<>
struct Engine::ECS::ComponentTraits<LifeTimeComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		LifeTimeComponent& _comp = Engine::Editor::GetValue<LifeTimeComponent>(a_pData);
		a_ar.Field("value", _comp.value);
	}

	static void Edit(CompEditContext& a_context)
	{
		LifeTimeComponent& _comp = Engine::Editor::GetValue<LifeTimeComponent>(a_context.pData);
		ImGui::DragFloat("LifeTime", &_comp.value, 0.1f);
		ImGui::TextDisabled("0 以下で消滅 / 負の値は無期限");
	}
};
