#pragma once

//==========================================================================================
// ChargeDashComponent
//
// ジャンプ(Space)を溜めてから撃ち出す「チャージダッシュ」の設定と進行状態。
//
// ・溜めと発動
//     押している間 chargeTime まで溜まり、溜まりきってから離すと発動する。
//     溜まりきる前に離したら何も起きない(いつも通りの上昇で終わる)ので、
//     普段の飛行操作はそのまま残る。
//     溜まりきった時点で自動的に出したいときは isAutoRelease を立てる。
//
// ・出てからの挙動
//     発動した瞬間の向きを dashDir に焼き付けて、そこへ dashSpeed で直進する。
//     ブースト(RobotBoostSystem)のように毎フレーム入力から向きを作り直さないので、
//     出ている間は曲がれない。速さもブーストとは別値で、倍以上を想定している。
//
// ・止まり方は2つ
//     (1) dashTime を使い切る
//     (2) 進んでいる向きと逆へ入力する(既定の操作なら S キー)
//         判定は「入力の向き」と dashDir の内積が brakeDot 以下か。
//         視点を回しても『進行方向の逆』で止まる形にしたいので、
//         キーそのものではなくワールドでの向きで見ている。
//   どちらで止まっても、いきなり 0 にはせず exitSpeedScale ぶんだけ速度を残す。
//   急停止させると、そのフレームだけ画も操作も固まって見えるため。
//
// ・値をここに置く理由
//     溜め時間・速さ・止まるまでの時間は手触りそのもので、
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
	float dashSpeed = 90.0f;		// ダッシュ中の速度(m/秒)。ブーストの boostPower より十分速く
	float dashTime = 0.6f;			// 何も入力しなくても止まるまでの秒数
	float coolTime = 0.5f;			// 止まってから次の溜めを始められるまでの秒数(0 なら即)

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
	// ダッシュ中は高さを保つ(重力と上下入力を無効にして水平へ直進する)
	bool isKeepHeight = true;

	// ---- 入力(ランタイム。InputMoveSystem が毎フレーム書く) ----
	bool isChargeIntent = false;	// 溜めボタンが押されているか
	bool isChargeRelease = false;	// 離した瞬間か

	// ---- ランタイム(保存しない) ----
	float chargeTimer = 0.0f;		// 溜まっている秒数(chargeTime で頭打ち)
	float charge01 = 0.0f;			// 溜まり具合 0〜1。演出はこれを見て大きくなる
	bool  isCharging = false;		// 今まさに溜めている最中か
	bool  isCharged = false;		// 溜まりきっているか(離せば出る状態)

	bool  isDashing = false;		// ダッシュ中か
	bool  isJustDashed = false;		// 出た瞬間のフレームか(演出の打ち上げ用)
	float dashTimer = 0.0f;			// 止まるまでの残り秒数
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
		a_ar.Field("dashTime", _comp.dashTime);
		a_ar.Field("coolTime", _comp.coolTime);

		a_ar.Field("brakeDot", _comp.brakeDot);
		a_ar.Field("exitSpeedScale", _comp.exitSpeedScale);

		a_ar.Field("isUseMoveDir", _comp.isUseMoveDir);
		a_ar.Field("isKeepHeight", _comp.isKeepHeight);
	}

	static void Edit(CompEditContext& a_context)
	{
		ChargeDashComponent& _comp = Engine::Editor::GetValue<ChargeDashComponent>(a_context.pData);

		ImGui::SeparatorText("Charge");
		ImGui::TextDisabled("ジャンプを押しっぱなしで溜める。溜まりきる前に離したら発動しない");
		ImGui::DragFloat("ChargeTime (s)", &_comp.chargeTime, 0.01f, 0.0f);
		ImGui::Checkbox("AutoRelease", &_comp.isAutoRelease);
		if (_comp.isAutoRelease)
		{
			ImGui::TextDisabled("溜まりきった瞬間に自動で発動する");
		}

		ImGui::SeparatorText("Dash");
		ImGui::TextDisabled("出た瞬間の向きへ直進する。出ている間は曲がれない");
		ImGui::DragFloat("DashSpeed (m/s)", &_comp.dashSpeed, 0.5f, 0.0f);
		ImGui::DragFloat("DashTime (s)", &_comp.dashTime, 0.01f, 0.0f);
		ImGui::DragFloat("CoolTime (s)", &_comp.coolTime, 0.01f, 0.0f);
		ImGui::Checkbox("UseMoveDir", &_comp.isUseMoveDir);
		ImGui::TextDisabled(_comp.isUseMoveDir
			? "移動入力があればその向き / 無ければ視点の正面"
			: "いつでも視点の正面へ出る");
		ImGui::Checkbox("KeepHeight", &_comp.isKeepHeight);
		if (_comp.isKeepHeight)
		{
			ImGui::TextDisabled("ダッシュ中は落ちない(水平へ直進する)");
		}

		ImGui::SeparatorText("Brake");
		ImGui::TextDisabled("進行方向の逆へ入力すると途中で止まる(既定の操作なら S)");
		ImGui::SliderFloat("BrakeDot", &_comp.brakeDot, -1.0f, 0.0f);
		ImGui::TextDisabled("-1 : 真後ろだけ / 0 に近いほど横入力でも止まる");
		ImGui::SliderFloat("ExitSpeedScale", &_comp.exitSpeedScale, 0.0f, 1.0f);
		ImGui::TextDisabled("止まった直後に残す速さの割合。0 で完全停止");

		// ランタイムは表示のみ
		ImGui::SeparatorText("Runtime");
		ImGui::ProgressBar(std::clamp(_comp.charge01, 0.0f, 1.0f), ImVec2(-FLT_MIN, 0),
			_comp.isCharged ? "Charged" : "Charging");
		ImGui::Text("Dashing   : %s", _comp.isDashing ? "true" : "false");
		ImGui::Text("DashTimer : %.3f", _comp.dashTimer);
		ImGui::Text("CoolTimer : %.3f", _comp.coolTimer);
		ImGui::Text("DashDir   : %.2f, %.2f, %.2f",
			_comp.dashDir.x, _comp.dashDir.y, _comp.dashDir.z);
	}
};
