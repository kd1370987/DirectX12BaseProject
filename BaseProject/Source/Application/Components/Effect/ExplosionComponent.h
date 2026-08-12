#pragma once

struct PartsEffect
{
	// エフェクトエンティティと発生タイミング
	Engine::GUID prefabGUID = {};							// 記録用(セーブされる)
	Engine::Handle<Engine::Resource::Prefab> prefabHandle = {};	// ランタイム用
	float emitTime = 0.0f;
};

/// <summary>
/// 発生のタイミングのみ握っておく。各自の動きや使うパーティクルアセットなどは各プレハブが管理する。
/// すべてのプレハブが動作したら、自身を消去する
/// </summary>
struct ExplosionComponent
{
	float elapsedTime = 0.0f;	// 経過時間
	float duration = 0.f;		// 間隔

	PartsEffect farstEffect = {};
	PartsEffect secondEffect = {};
	PartsEffect theardEffect = {};
};

template<>
struct Engine::ECS::ComponentTraits<ExplosionComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ExplosionComponent& _comp = Engine::Editor::GetValue<ExplosionComponent>(a_pData);
	}

	static void Edit(CompEditContext& a_context)
	{
		ExplosionComponent& _comp = Engine::Editor::GetValue<ExplosionComponent>(a_context.pData);
	}
};