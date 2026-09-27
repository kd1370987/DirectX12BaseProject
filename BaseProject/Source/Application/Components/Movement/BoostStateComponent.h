#pragma once

//==========================================================================================
// BoostStateComponent
//
// ブーストの実行中の状態(ブースト中か・燃料・踏み込みの残り)。
//
// ・書くのは RobotBoostSystem と ChargeDashSystem(ダッシュの燃料)。順序は明示で決めてある。
//   読むのは見た目・音・UI(Thruster / BoosterEffect / BoostSound / UIGauge)。
// ・BoostParamsComponent の必須コンポーネントなので、プレハブに書かなくても付く。保存しない。
//==========================================================================================
struct BoostStateComponent
{
	bool isBoosting = false;		// 実際に現在ブースト中か(燃料切れなどで押してても飛べない場合があるため)

	float currentFuel = 100.0f;		// 現在の燃料/エネルギー

	// --- 初動の状態 ---
	float tapBoostTimer = 0.0f;							// 初動が残っている秒数
	Math::Vector3 tapBoostDir = { 0.0f, 0.0f, 0.0f };	// 蹴り出した向き(水平)
};

template<>
struct Engine::ECS::ComponentTraits<BoostStateComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		BoostStateComponent& _comp = Engine::Editor::GetValue<BoostStateComponent>(a_context.pData);
		Engine::Editor::Field("Is Boosting (Active)", _comp.isBoosting);
		Engine::Editor::Value("Fuel", "%.1f", _comp.currentFuel);
		Engine::Editor::Value("TapBoostTimer", "%.2f", _comp.tapBoostTimer);
	}
};
