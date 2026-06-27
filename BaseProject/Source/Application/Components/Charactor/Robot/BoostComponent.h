#pragma once

struct BoostComponent
{
	// --- 入力・状態 ---
	bool isBoostTriger = false;		// ブーストボタンが押された瞬間
	bool isJustBoosted = false;		// ブーストボタンが押された瞬間
	bool isBoostIntent = false;		// ブーストボタンが押されているか
	bool isBoosting = false;		// 実際に現在ブースト中か（燃料切れなどで押してても飛べない場合があるため）

	// --- パラメータ ---
	float boostPower = 20.0f;		// ブースト時の推力
	float boostFuel = 5.0f;			// ブースト単押しの使用燃料
	float boostFuelPerSec = 1.0f;	// ブースト連続使用時の毎秒消費燃料量

	float currentFuel = 100.0f;		// 現在の燃料/エネルギー
	float maxFuel = 100.0f;			// 燃料の最大値
	float fuelRegeneration = 1.0f;	// 秒間回復量
};

template<>
struct Engine::ECS::ComponentTraits<BoostComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		BoostComponent& _comp = Engine::Editor::GetValue<BoostComponent>(a_pData);
		a_ar.Field("maxFuel", _comp.maxFuel);
		a_ar.Field("boostPower", _comp.boostPower);
		a_ar.Field("boostFuel", _comp.boostFuel);
		a_ar.Field("boostFuelPerSec", _comp.boostFuelPerSec);
		a_ar.Field("fuelRegeneration", _comp.fuelRegeneration);
	}

	static void Edit(void* a_pData)
	{
		using namespace Engine;
		BoostComponent& _comp = Engine::Editor::GetValue<BoostComponent>(a_pData);

		ImGui::Text("Boost Parameters");
		ImGui::DragFloat("Max Fuel", &_comp.maxFuel, 1.0f, 0.0f);
		ImGui::DragFloat("Boost Power", &_comp.boostPower, 0.1f, 0.0f);
		ImGui::DragFloat("Boost Fuel (Tap)", &_comp.boostFuel, 0.1f, 0.0f);
		ImGui::DragFloat("Boost Fuel / Sec", &_comp.boostFuelPerSec, 0.1f, 0.0f);

		ImGui::Separator();

		ImGui::Text("Runtime State");
		ImGui::Checkbox("Boost Triger (Input)", &_comp.isJustBoosted);
		ImGui::Checkbox("Boost Intent (Input)", &_comp.isBoostIntent);
		ImGui::Checkbox("Is Boosting (Active)", &_comp.isBoosting);

		// 現在の燃料残量を可視化するプログレスバー（maxFuelが0の時のゼロ除算を防止）
		float fraction = (_comp.maxFuel > 0.0f) ? (_comp.currentFuel / _comp.maxFuel) : 0.0f;
		char overlay[32];
		snprintf(overlay, sizeof(overlay), "%.1f / %.1f", _comp.currentFuel, _comp.maxFuel);
		ImGui::ProgressBar(fraction, ImVec2(-FLT_MIN, 0), overlay);
		ImGui::DragFloat("FuelRegeneration", &_comp.fuelRegeneration, 0.1f, 0.0f);
	}
};