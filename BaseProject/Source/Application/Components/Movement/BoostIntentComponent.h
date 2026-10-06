#pragma once

namespace App::Component
{
	//==========================================================================================
	// BoostIntentComponent
	//
	// ブーストの入力(押した瞬間・押しているか)。
	//
	// ・書くのは入力を作る側 : InputMoveSystem(プレイヤー)、BossCombatIntentSystem(ボス)、
	//   DeathBoostGateSystem(死亡中は消す)。
	//   読むのは RobotBoostSystem / ChargeDashSystem と、見た目・音(Thruster / BoostSound)。
	// ・BoostParamsComponent の必須コンポーネントなので、プレハブに書かなくても付く。保存しない。
	//==========================================================================================
	struct BoostIntentComponent
	{
		bool isBoostTriger = false;		// ブーストボタンが押された瞬間
		bool isBoostIntent = false;		// ブーストボタンが押されているか
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::BoostIntentComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		App::Component::BoostIntentComponent& _comp = Engine::EditorField::GetValue<App::Component::BoostIntentComponent>(a_context.pData);
		Engine::EditorField::Field("Boost Triger (Input)", _comp.isBoostTriger);
		Engine::EditorField::Field("Boost Intent (Input)", _comp.isBoostIntent);
	}
};
