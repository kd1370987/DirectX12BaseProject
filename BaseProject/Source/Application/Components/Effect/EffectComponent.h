#pragma once
/// <summary>
/// エフェクトとして生成されるエンティティに付与する目印
///
/// 寿命は LifeTimeComponent が持つので、時間で消えるエフェクトには
/// そちらも一緒に付ける(消すのは LifeTimeSystem)。
/// こちらは「このエンティティはエフェクトである」という区別だけを持つ。
/// 現状データは無いが、エフェクトだけをまとめて扱いたくなったとき
/// (一括停止・演出のスキップなど)の絞り込みに使える。
/// </summary>
struct EffectComponent
{

};

template<>
struct Engine::ECS::ComponentTraits<EffectComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
	}

	static void Edit(CompEditContext& a_context)
	{
		ImGui::TextDisabled("Marker only. LifeTime is LifeTimeComponent.");
	}
};
