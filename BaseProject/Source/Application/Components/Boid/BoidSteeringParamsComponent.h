#pragma once

#include "BoidMembershipComponent.h"
#include "BoidTargetComponent.h"

namespace App::Component
{
	//==========================================================================================
	// BoidSteeringParamsComponent
	//
	// 群れの操舵(分離・整列・結合・目標への追従)と向きの設定。保存される。
	// システムは読むだけ(BoidSteeringSystem / SwarmLookSystem)。
	//
	// ・以前の BoidComponent から、所属(BoidMembershipComponent)と
	//   目標地点(BoidTargetComponent)を分けた残り。どちらも必須コンポーネントとして自動で付く。
	// ・maxSpeed / maxSteeringForce は SwarmBossController が生成時にリーダーの速さから上書きする。
	//==========================================================================================
	struct BoidSteeringParamsComponent
	{
		float slowRadius = 0.0f;				// 目標地点にどの程度近づいたら減速を入れるか
		float maxSpeed = 1.0f;					// 最高速度
		float seekWeight = 0.0f;				// 目標地点へ向かう力の重さ

		float neighborRadius = 0.0f;			// 整列・結合の相手に数える範囲
		float separationDistance = 0.0f;		// 反発力を受けなくなる境界
		float separationWeight = 1.0f;			// 反発力の重さ

		float alignmentWeight = 1.0f;			// 周囲の進行速度の重さ
		float cohesionWeight = 1.0f;			// 周囲の平均位置へ向かう重さ

		float maxSteeringForce = 1.0f;			// 力の最大値

		// 向き : 所属している小隊長の向きへ寄せる速さ(度/秒)
		// 寄せるのは SwarmLookSystem。体の向きにするのは RotationSystem
		float turnSpeedDeg = 540.0f;
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::BoidSteeringParamsComponent>
{
	// 所属と目標地点は実行中の値なので、プレハブに書かずに自動で付ける
	using Requires = Engine::ECS::RequireComponents<App::Component::BoidMembershipComponent, App::Component::BoidTargetComponent>;

	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::BoidSteeringParamsComponent& _comp = Engine::EditorField::GetValue<App::Component::BoidSteeringParamsComponent>(a_pData);

		a_ar.Field("slowRadius", _comp.slowRadius);
		a_ar.Field("maxSpeed", _comp.maxSpeed);
		a_ar.Field("seekWeight", _comp.seekWeight);

		a_ar.Field("neighborRadius", _comp.neighborRadius);
		a_ar.Field("separationDistance", _comp.separationDistance);
		a_ar.Field("separationWeight", _comp.separationWeight);

		a_ar.Field("alignmentWeight", _comp.alignmentWeight);
		a_ar.Field("cohesionWeight", _comp.cohesionWeight);

		a_ar.Field("maxSteeringForce", _comp.maxSteeringForce);
		a_ar.Field("turnSpeedDeg", _comp.turnSpeedDeg);
	}

	static void Edit(CompEditContext& a_context)
	{
		App::Component::BoidSteeringParamsComponent& _comp = Engine::EditorField::GetValue<App::Component::BoidSteeringParamsComponent>(a_context.pData);
		Engine::EditorField::Field("slowRadius", _comp.slowRadius);
		Engine::EditorField::Field("maxSpeed", _comp.maxSpeed);
		Engine::EditorField::Field("seekWeight", _comp.seekWeight);
		Engine::EditorField::Line();
		Engine::EditorField::Field("neighborRadius", _comp.neighborRadius);
		Engine::EditorField::Field("separationDistance", _comp.separationDistance);
		Engine::EditorField::Field("separationWeight", _comp.separationWeight);
		Engine::EditorField::Line();
		Engine::EditorField::Field("alignmentWeight", _comp.alignmentWeight);
		Engine::EditorField::Field("cohesionWeight", _comp.cohesionWeight);
		Engine::EditorField::Line();
		Engine::EditorField::Field("maxSteeringForce", _comp.maxSteeringForce);
		Engine::EditorField::Line();
		Engine::EditorField::Field("turnSpeedDeg", _comp.turnSpeedDeg);
	}
};
