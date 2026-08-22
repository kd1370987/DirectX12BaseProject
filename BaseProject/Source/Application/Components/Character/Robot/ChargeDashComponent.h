#pragma once

//==========================================================================================
// ChargeDashComponent
//
// ジャンプ(Space)を溜めてから撃ち出す「チャージダッシュ」の設定と進行状態。
// Space は溜め専用のボタンで、押しても上昇はしない(上昇はブースト側の担当)。
//
// ・溜めと発動
//     押している間 chargeTime まで溜まり、溜まりきってから離すと発動する。
//     溜まりきる前に離したら何も起きない。
//     溜まりきった時点で自動的に出したいときは isAutoRelease を立てる。
//
// ・出てからの挙動
//     発動した瞬間の向きを dashDir に焼き付けて、そこへ dashSpeed で直進する。
//     ブースト(RobotBoostSystem)のように毎フレーム入力から向きを作り直さないので、
//     進む軸そのものは変えられない(＝曲がれない)。
//
//     ただし操作をまるごと奪うわけではなく、進む軸から外れた向きへは動かせる。
//       横 : 入力から進行軸の成分を抜いた残りぶんだけ、dashStrafeSpeed で流れる
//       上下 : 上昇(Space)/急降下(LCtrl)が dashVerticalSpeed で効く
//     突っ込みながら軸をずらして避ける、という動きができる。
//     軸そのものが変わらないので「曲がれない」感触は残る。
//
// ・止まり方は2つ。時間では止まらない
//     (1) エネルギーが尽きる
//         BoostComponent の燃料を dashFuelPerSec で吸い続け、0 になったら終わり。
//         ブーストと同じエネルギーを食うので、飛ぶか突っ込むかの選択になる。
//         ※ BoostComponent を持たない相手は尽きないので (2) でしか止まらない
//     (2) 進んでいる向きと逆へ入力する(既定の操作なら S キー)
//         判定は「入力の向き」と dashDir の内積が brakeDot 以下か。
//         視点を回しても『進行方向の逆』で止まる形にしたいので、
//         キーそのものではなくワールドでの向きで見ている。
//         横入力で止まってしまわないよう、しきい値は 0 から離しておくこと。
//   どちらで止まっても、いきなり 0 にはせず exitSpeedScale ぶんだけ速度を残す。
//   急停止させると、そのフレームだけ画も操作も固まって見えるため。
//
// ・値をここに置く理由
//     溜め時間・速さ・消費エネルギーは手触りそのもので、
//     コードを触らずに詰めたい類のもの。演出の大きさ(溜め中に太る/出た瞬間に伸びる)は
//     ブースター側の見た目なので BoosterEffectComponent が持ち、
//     こちらは「今どれだけ溜まったか(charge01)」「出ているか(isDashing)」だけを配る。
//
// 進行を回すのは ChargeDashSystem(Physics)。
// 入力を書くのは InputMoveSystem(Input)、演出へ配るのは ThrusterEffectSystem(PreUpdate)。
//==========================================================================================
struct ChargeDashComponent
{
	// ---- 溜め(設定値) ----
	float chargeTime = 0.8f;		// 溜まりきるまでの秒数。これより早く離したら発動しない
	bool  isAutoRelease = false;	// true : 溜まりきった瞬間に自動で発動する(離すのを待たない)

	// ---- ダッシュ(設定値) ----
	float dashSpeed = 90.0f;		// 進行軸の速度(m/秒)。ブーストの boostPower より十分速く
	float dashStrafeSpeed = 25.0f;	// 進行軸から外れた向きへ流れる速度(m/秒)。0 で真っ直ぐしか動けない
	float dashVerticalSpeed = 25.0f;// 上下入力で動く速度(m/秒)
	float coolTime = 0.5f;			// 止まってから次の溜めを始められるまでの秒数(0 なら即)

	// ---- 消費エネルギー(設定値) ----
	// BoostComponent の燃料を毎秒この量だけ吸う。0 にすると尽きなくなる。
	// 燃料は毎秒 fuelRegeneration ぶん回復し続けるので、
	// 実際に減る量はその差ぶん。回復量より小さいと永久に飛べてしまう
	float dashFuelPerSec = 150.0f;

	// ---- 止め方(設定値) ----
	// 入力の向きと dashDir の内積がこれ以下なら「逆入力」とみなして途中停止する。
	// -1 に近いほど真後ろだけ、0 に近いほど横入力でも止まる
	float brakeDot = -0.5f;
	// 止まった直後に残す速さの割合(dashSpeed に対して)。0 で完全停止
	float exitSpeedScale = 0.2f;

	// ---- 向き(設定値) ----
	// true  : 移動入力があればその向きへ、無ければ視点の正面へ出る
	// false : いつでも視点の正面へ出る
	bool isUseMoveDir = true;
	// ダッシュ中は重力を無効にする(上下は入力ぶんだけ動く)。
	// false なら落ちながらのダッシュになり、上下入力は重力に足される
	bool isKeepHeight = true;
	// ダッシュ中だけ Space を上昇に使うか。
	// 溜めに使うボタンと同じだが、出ている間は溜められないので取り合いにならない。
	// false にすると上下は急降下(LCtrl)だけになる
	bool isUseJumpAscend = true;

	// ---- 入力(ランタイム。InputMoveSystem が毎フレーム書く) ----
	bool isChargeIntent = false;	// 溜めボタン(Space)が押されているか
	bool isChargeRelease = false;	// 離した瞬間か

	// ---- ランタイム(保存しない) ----
	float chargeTimer = 0.0f;		// 溜まっている秒数(chargeTime で頭打ち)
	float charge01 = 0.0f;			// 溜まり具合 0〜1。演出はこれを見て大きくなる
	bool  isCharging = false;		// 今まさに溜めている最中か
	bool  isCharged = false;		// 溜まりきっているか(離せば出る状態)

	bool  isDashing = false;		// ダッシュ中か
	bool  isJustDashed = false;		// 出た瞬間のフレームか(演出の打ち上げ用)
	float dashElapsed = 0.0f;		// 出てからの経過秒数(表示・調整用。止める判断には使わない)
	float coolTimer = 0.0f;			// 次の溜めを始められるまでの残り秒数

	Math::Vector3 dashDir = { 0.0f, 0.0f, 1.0f };	// 発動した瞬間に焼き付けた進行方向(水平)
};

template<>
struct Engine::ECS::ComponentTraits<ChargeDashComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		ChargeDashComponent& _comp = Engine::Editor::GetValue<ChargeDashComponent>(a_pData);

		a_ar.Field("chargeTime", _comp.chargeTime);
		a_ar.Field("isAutoRelease", _comp.isAutoRelease);

		a_ar.Field("dashSpeed", _comp.dashSpeed);
		a_ar.Field("dashStrafeSpeed", _comp.dashStrafeSpeed);
		a_ar.Field("dashVerticalSpeed", _comp.dashVerticalSpeed);
		a_ar.Field("coolTime", _comp.coolTime);

		a_ar.Field("dashFuelPerSec", _comp.dashFuelPerSec);

		a_ar.Field("brakeDot", _comp.brakeDot);
		a_ar.Field("exitSpeedScale", _comp.exitSpeedScale);

		a_ar.Field("isUseMoveDir", _comp.isUseMoveDir);
		a_ar.Field("isKeepHeight", _comp.isKeepHeight);
		a_ar.Field("isUseJumpAscend", _comp.isUseJumpAscend);
	}

	static void Edit(CompEditContext& a_context)
	{
		ChargeDashComponent& _comp = Engine::Editor::GetValue<ChargeDashComponent>(a_context.pData);

		ImGui::SeparatorText("Charge");
		ImGui::TextDisabled("Space を押しっぱなしで溜める。溜まりきる前に離したら発動しない");
		ImGui::DragFloat("ChargeTime (s)", &_comp.chargeTime, 0.01f, 0.0f);
		ImGui::Checkbox("AutoRelease", &_comp.isAutoRelease);
		if (_comp.isAutoRelease)
		{
			ImGui::TextDisabled("溜まりきった瞬間に自動で発動する");
		}

		ImGui::SeparatorText("Dash");
		ImGui::TextDisabled("出た瞬間の向きへ直進する。進む軸は変えられない(曲がれない)");
		ImGui::DragFloat("DashSpeed (m/s)", &_comp.dashSpeed, 0.5f, 0.0f);
		ImGui::DragFloat("StrafeSpeed (m/s)", &_comp.dashStrafeSpeed, 0.5f, 0.0f);
		ImGui::TextDisabled("進行軸から外れた向きへ流れる速さ。0 で真っ直ぐしか動けない");
		ImGui::DragFloat("VerticalSpeed (m/s)", &_comp.dashVerticalSpeed, 0.5f, 0.0f);
		ImGui::DragFloat("CoolTime (s)", &_comp.coolTime, 0.01f, 0.0f);
		ImGui::Checkbox("UseMoveDir", &_comp.isUseMoveDir);
		ImGui::TextDisabled(_comp.isUseMoveDir
			? "移動入力があればその向き / 無ければ視点の正面"
			: "いつでも視点の正面へ出る");
		ImGui::Checkbox("KeepHeight", &_comp.isKeepHeight);
		ImGui::TextDisabled(_comp.isKeepHeight
			? "ダッシュ中は落ちない(上下は入力ぶんだけ)"
			: "落ちながら進む(上下入力は重力に足される)");
		ImGui::Checkbox("UseJumpAscend", &_comp.isUseJumpAscend);
		ImGui::TextDisabled(_comp.isUseJumpAscend
			? "ダッシュ中だけ Space が上昇になる"
			: "上下は急降下(LCtrl)だけ");

		ImGui::SeparatorText("Energy");
		ImGui::TextDisabled("ブーストと同じ燃料(BoostComponent)を吸う。尽きたら止まる");
		ImGui::DragFloat("FuelPerSec", &_comp.dashFuelPerSec, 1.0f, 0.0f);
		if (_comp.dashFuelPerSec <= 0.0f)
		{
			ImGui::TextDisabled("0 : エネルギーを消費しない(逆入力でしか止まらない)");
		}
		else
		{
			ImGui::TextDisabled("回復量(FuelRegeneration)より大きくしないと尽きません");
		}

		ImGui::SeparatorText("Brake");
		ImGui::TextDisabled("進行方向の逆へ入力すると止まる(既定の操作なら S)");
		ImGui::SliderFloat("BrakeDot", &_comp.brakeDot, -1.0f, 0.0f);
		ImGui::TextDisabled("-1 : 真後ろだけ / 0 に近いほど横入力でも止まる");
		ImGui::SliderFloat("ExitSpeedScale", &_comp.exitSpeedScale, 0.0f, 1.0f);
		ImGui::TextDisabled("止まった直後に残す速さの割合。0 で完全停止");

		// ランタイムは表示のみ
		ImGui::SeparatorText("Runtime");
		ImGui::ProgressBar(std::clamp(_comp.charge01, 0.0f, 1.0f), ImVec2(-FLT_MIN, 0),
			_comp.isCharged ? "Charged" : "Charging");
		ImGui::Text("Dashing     : %s", _comp.isDashing ? "true" : "false");
		ImGui::Text("DashElapsed : %.3f", _comp.dashElapsed);
		ImGui::Text("CoolTimer   : %.3f", _comp.coolTimer);
		ImGui::Text("DashDir     : %.2f, %.2f, %.2f",
			_comp.dashDir.x, _comp.dashDir.y, _comp.dashDir.z);
	}
};
