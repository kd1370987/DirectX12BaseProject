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


	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		using namespace Engine;
		auto* _comp = static_cast<const BoostComponent*>(a_ptr);

		// 状態（isBoosting等）や変動する現在値（currentFuel）は保存しない
		JSONHelper::SetValue("maxFuel", a_json, _comp->maxFuel);
		JSONHelper::SetValue("boostPower", a_json, _comp->boostPower);
		JSONHelper::SetValue("boostFuel", a_json, _comp->boostFuel);
		JSONHelper::SetValue("boostFuelPerSec", a_json, _comp->boostFuelPerSec);
		JSONHelper::SetValue("fuelRegeneration", a_json, _comp->fuelRegeneration);
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		using namespace Engine;
		auto* _comp = static_cast<BoostComponent*>(a_ptr);

		// JSONからパラメータを読み込み（見つからなければ右側のデフォルト値を使用）
		_comp->maxFuel = JSONHelper::GetValue("maxFuel", a_json, 100.0f);
		_comp->boostPower = JSONHelper::GetValue("boostPower", a_json, 20.0f);
		_comp->boostFuel = JSONHelper::GetValue("boostFuel", a_json, 5.0f);
		_comp->boostFuelPerSec = JSONHelper::GetValue("boostFuelPerSec", a_json, 1.0f);
		_comp->fuelRegeneration = JSONHelper::GetValue("fuelRegeneration", a_json, 1.0f);

		// ロード完了時に現在の燃料を最大値まで回復させておく
		_comp->currentFuel = _comp->maxFuel;
		_comp->isBoostIntent = false;
		_comp->isBoosting = false;
	}

	static void Edit(void* a_data)
	{
		using namespace Engine;
		BoostComponent& _comp = Engine::Editor::GetValue<BoostComponent>(a_data);

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