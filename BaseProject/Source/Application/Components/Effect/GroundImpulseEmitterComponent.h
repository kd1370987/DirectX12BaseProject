#pragma once

#include "Engine/EditorField/EditorField.h"

namespace App::Component
{
	//==========================================================================================
	// GroundImpulseEmitterComponent
	//
	// 自分の位置からグラウンドフィールドへ衝撃を1発出す(テスト用)。
	// 出現した瞬間に1発出し、lifetime を過ぎたら止まる。
	// エディターのボタンからもう一度出し直せる。
	//
	// ・衝撃は SceneView に毎フレーム積み直す必要があるので、
	//   出している間は GroundImpulseEmitSystem が経過時間を進めながら積み続ける。
	// ・位置は WorldMatrix の平行移動成分。
	//==========================================================================================
	struct GroundImpulseEmitterComponent
	{
		// 衝撃の設定(保存する)
		float radius = 1.0f;		// 波の厚み
		float strength = 1.0f;		// 強さ
		float speed = 10.0f;		// 波が広がる速さ(m/秒)
		float width = 1.0f;			// 幅
		float lifetime = 3.0f;		// 出し続ける時間(秒)

		// ランタイム(保存しない)
		bool isFired = false;		// 出現時の1発を出したか
		float age = -1.0f;			// 出してからの経過時間(秒)。負なら出していない
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::GroundImpulseEmitterComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::GroundImpulseEmitterComponent& _comp = Engine::EditorField::RefValue<App::Component::GroundImpulseEmitterComponent>(a_pData);

		// 発射済みフラグと経過時間はランタイム状態なので保存しない
		a_ar.Field("radius", _comp.radius);
		a_ar.Field("strength", _comp.strength);
		a_ar.Field("speed", _comp.speed);
		a_ar.Field("width", _comp.width);
		a_ar.Field("lifetime", _comp.lifetime);
	}

	static void Edit(CompEditContext& a_context)
	{
		App::Component::GroundImpulseEmitterComponent& _comp = Engine::EditorField::RefValue<App::Component::GroundImpulseEmitterComponent>(a_context.pData);

		Engine::EditorField::Field("radius", _comp.radius, 0.05f, 0.0f);
		Engine::EditorField::Tooltip("波の厚み");
		Engine::EditorField::Field("strength", _comp.strength, 0.05f);
		Engine::EditorField::Field("speed", _comp.speed, 0.1f, 0.0f);
		Engine::EditorField::Tooltip("波が広がる速さ(m/秒)");
		Engine::EditorField::Field("width", _comp.width, 0.05f, 0.0f);
		Engine::EditorField::Field("lifetime", _comp.lifetime, 0.05f, 0.0f);
		Engine::EditorField::Tooltip("出し続ける時間(秒)");

		Engine::EditorField::Header("Debug");
		if (Engine::EditorField::Button("Fire"))
		{
			// 経過時間を戻すと、次のフレームから出し直しになる
			_comp.isFired = true;
			_comp.age = 0.0f;
		}
		Engine::EditorField::Tooltip("自分の位置から衝撃をもう一度出す");

		if (_comp.age >= 0.0f)
		{
			Engine::EditorField::Value("age", "%.2f / %.2f", _comp.age, _comp.lifetime);
		}
	}
};
