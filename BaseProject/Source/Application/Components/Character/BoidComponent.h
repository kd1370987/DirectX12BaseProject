#pragma once

struct BoidComponent
{
	Engine::ECS::Entity platoonID = Engine::ECS::Limits::INVALID_ENTITY;

	Math::Vector3 targetPos;				// 目標地点
	float pow = 0.0f;						// 目標地点への追従強度
	float slowRadius = 0.0f;				// 目標地点にどの程度近づいたら減速を入れるか
	float maxSpeed = 1.0f;					// ムーブメントコンポーネント側に移行予定。テスト用
	float seekWeight = 0.0f;				// 現在速度との差のウェイト

	float distanceLenge = 0.0f;				// 反発する長さ
	float separationDistance = 0.0f;		// 反発力を受けなくなる境界
	float separationWeight = 1.0f;			// 反発力の重さ

	float neighborDistance = 0.0f;			// 周囲のボイドと軍隊行動をする範囲
	float alignmentWeight = 1.0f;			// 周囲の進行速度の重さ
	float cohesionWeight = 1.0f;			// 周囲の平均位置へ向かう重さ

	float maxSteeringForce = 1.0f;			// 力の最大値
};

template<>
struct Engine::ECS::ComponentTraits<BoidComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		BoidComponent& _comp = Engine::Editor::GetValue<BoidComponent>(a_pData);

		a_ar.Field("targetPos", _comp.targetPos);
		a_ar.Field("pow", _comp.pow);
		a_ar.Field("slowRadius", _comp.slowRadius);
		a_ar.Field("maxSpeed", _comp.maxSpeed);
		a_ar.Field("seekWeight", _comp.seekWeight);

		a_ar.Field("distanceLenge", _comp.distanceLenge);
		a_ar.Field("separationDistance", _comp.separationDistance);
		a_ar.Field("separationWeight", _comp.separationWeight);

		a_ar.Field("neighborDistance", _comp.neighborDistance);
		a_ar.Field("alignmentWeight", _comp.alignmentWeight);
		a_ar.Field("cohesionWeight", _comp.cohesionWeight);

		a_ar.Field("maxSteeringForce", _comp.maxSteeringForce);
	}

	static void Edit(CompEditContext& a_context)
	{
		BoidComponent& _comp = Engine::Editor::GetValue<BoidComponent>(a_context.pData);
		ImGui::DragFloat3("targetPos",&_comp.targetPos.x);
		ImGui::DragFloat("pow",&_comp.pow);
		ImGui::DragFloat("slowRadius",&_comp.slowRadius);
		ImGui::DragFloat("maxSpeed",&_comp.maxSpeed);
		ImGui::DragFloat("seekWeight",&_comp.seekWeight);
		ImGui::Separator();
		ImGui::DragFloat("DistanceLenge", &_comp.distanceLenge);
		ImGui::DragFloat("separationDistance",&_comp.separationDistance);
		ImGui::DragFloat("separationWeight",&_comp.separationWeight);
		ImGui::Separator();
		ImGui::DragFloat("neighborDistance",&_comp.neighborDistance);
		ImGui::DragFloat("alignmentWeight",&_comp.alignmentWeight);
		ImGui::DragFloat("cohesionWeight",&_comp.cohesionWeight);
		ImGui::Separator();
		ImGui::DragFloat("maxSteeringForce",&_comp.maxSteeringForce);

	}
};