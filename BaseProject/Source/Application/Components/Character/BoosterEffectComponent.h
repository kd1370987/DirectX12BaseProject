#pragma once

struct BoosterEffectComponent
{
	// ブースターのジェットアセット
	Engine::GUID effectGUID = Engine::DefaultGUID;
	Engine::Handle<Engine::Resource::EffectAsset> effectHandle = {};
	Engine::Resource::EffectInstance instance = {};

	// ブースト発動時のスケール倍率
	float startScale = 1.0f;
	float duration = 0.3f;			// どの程度の時間をかけてスケールを1に戻すか

	// ブースト開始地点オフセットと、エフェクト発生方向
	Math::Vector3 pos = {};
	Math::Vector3 dir = {};
};

template<>
struct Engine::ECS::ComponentTraits<BoosterEffectComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		BoosterEffectComponent& _comp = Engine::Editor::GetValue<BoosterEffectComponent>(a_pData);

	}

	static void Edit(CompEditContext& a_context)
	{
		BoosterEffectComponent& _comp = Engine::Editor::GetValue<BoosterEffectComponent>(a_context.pData);
		
	}
};
