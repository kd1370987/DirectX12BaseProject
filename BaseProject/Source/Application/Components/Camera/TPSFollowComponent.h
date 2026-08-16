#pragma once

//==========================================================================================
// TPSFollowComponent
//
// TPSカメラの「追従の質」を決めるパラメータ。カメラ本体に付ける。
//
// 旧実装はピボットも注視点もターゲット座標をそのまま毎フレーム代入していて、
// 補間係数もコード内の固定値だった。結果として機体に完全に貼り付いた硬いカメラになる。
// アーマードコアのカメラは「少し遅れて付いてきて、速度が乗ると後ろへ引き、
// 上下方向はさらに柔らかく遅れる」という挙動なので、その調整値をここへ出す。
//
// 追従はフレームレート非依存の指数減衰で行う:
//     t = 1 - exp(-rate * dt)
// rate が大きいほど速く追いつき、0 にすると追従しない(置いていかれる)。
//==========================================================================================
struct TPSFollowComponent
{
	// ---- 追従速度(1秒あたりの減衰レート) ----
	float posRateHorizontal	= 9.0f;		// ピボットの水平追従。小さいほど機体が先行して見える
	float posRateVertical	= 4.0f;		// ピボットの垂直追従。水平より遅くすると上昇/落下が柔らかくなる
	float lookAtRate		= 12.0f;	// 注視点の追従
	float orbitRate			= 10.0f;	// オービット回転(視点の向き)の追従

	// ---- 速度レスポンス ----
	// 「今どれくらい速いか」を 0..1 に正規化して、引き・追従・画角へまとめて効かせる。
	//     speed   = |水平速度| + |上下速度| × verticalSpeedWeight を合成した速さ
	//     speed01 = clamp(speed / speedReference, 0, 1)
	// 空中戦では上下の速さも臨場感に効くので混ぜるが、ただ落ちているだけで
	// カメラが暴れないように重みで加減する(0 にすれば従来どおり水平のみ)。
	float speedReference		= 30.0f;	// 全開とみなす速度(m/s)。ブースト速度あたりが目安
	float verticalSpeedWeight	= 0.6f;		// 速度へ上下成分を混ぜる割合(0=水平のみ)

	// ---- 速度に応じた引き ----
	float speedPullBack		= 0.15f;	// 速度1あたり何m後ろへ引くか
	float maxPullBack		= 5.0f;		// 引きの上限(m)
	float pullBackRate		= 3.0f;		// 引き量そのものの追従速度(伸び/戻りの滑らかさ)

	// ---- 全開時(speed01 = 1)の効き ----
	// どれも停止時の値から speed01 で線形に寄せる。速いほど「置いていかれて」
	// 機体が画面の進行方向側へ流れ、画角が広がってスピード感が出る。
	float followRateScale	= 0.4f;		// 追従レート(位置/注視点)に掛ける倍率。小さいほど遅れる
	float maxLagAtSpeed		= 18.0f;	// 許すピボットの遅れ(m)。停止時は maxLagDistance
	float lookAtLagRatio	= 0.65f;	// 注視点の基準を「遅れたピボット」へ寄せる割合(0..1)
										// 0 なら常に機体が画面中央。上げるほど画面端へ流れて見切れる
	float fovAddAtSpeed		= 22.0f;	// 足す画角(度)。CameraParamComponent.fovY への加算
	float fovRate			= 5.0f;		// 画角の追従レート

	// ---- 遅れの上限 ----
	float maxLagDistance	= 8.0f;		// 停止時にピボットがターゲットから離れてよい最大距離(m)
										// テレポートや極端な加速でカメラが千切れないための保険
};

template<>
struct Engine::ECS::ComponentTraits<TPSFollowComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		TPSFollowComponent& _comp = Engine::Editor::GetValue<TPSFollowComponent>(a_pData);
		a_ar.Field("posRateHorizontal", _comp.posRateHorizontal);
		a_ar.Field("posRateVertical", _comp.posRateVertical);
		a_ar.Field("lookAtRate", _comp.lookAtRate);
		a_ar.Field("orbitRate", _comp.orbitRate);
		a_ar.Field("speedPullBack", _comp.speedPullBack);
		a_ar.Field("maxPullBack", _comp.maxPullBack);
		a_ar.Field("pullBackRate", _comp.pullBackRate);
		a_ar.Field("maxLagDistance", _comp.maxLagDistance);

		// 速度レスポンス。旧データにキーが無い場合は既定値のまま読み飛ばされる。
		a_ar.Field("speedReference", _comp.speedReference);
		a_ar.Field("verticalSpeedWeight", _comp.verticalSpeedWeight);
		a_ar.Field("followRateScale", _comp.followRateScale);
		a_ar.Field("maxLagAtSpeed", _comp.maxLagAtSpeed);
		a_ar.Field("lookAtLagRatio", _comp.lookAtLagRatio);
		a_ar.Field("fovAddAtSpeed", _comp.fovAddAtSpeed);
		a_ar.Field("fovRate", _comp.fovRate);
	}

	static void Edit(CompEditContext& a_context)
	{
		TPSFollowComponent& _comp = Engine::Editor::GetValue<TPSFollowComponent>(a_context.pData);

		ImGui::TextDisabled("Follow Rate");
		ImGui::DragFloat("PosRateH", &_comp.posRateHorizontal, 0.1f, 0.0f, 60.0f);
		ImGui::DragFloat("PosRateV", &_comp.posRateVertical, 0.1f, 0.0f, 60.0f);
		ImGui::DragFloat("LookAtRate", &_comp.lookAtRate, 0.1f, 0.0f, 60.0f);
		ImGui::DragFloat("OrbitRate", &_comp.orbitRate, 0.1f, 0.0f, 60.0f);

		ImGui::Separator();
		ImGui::TextDisabled("Speed Response");
		ImGui::DragFloat("SpeedReference", &_comp.speedReference, 0.5f, 0.1f, 500.0f);
		ImGui::DragFloat("VerticalSpeedWeight", &_comp.verticalSpeedWeight, 0.01f, 0.0f, 1.0f);

		ImGui::Separator();
		ImGui::TextDisabled("Speed Pull Back");
		ImGui::DragFloat("SpeedPullBack", &_comp.speedPullBack, 0.01f, 0.0f, 5.0f);
		ImGui::DragFloat("MaxPullBack", &_comp.maxPullBack, 0.1f, 0.0f, 50.0f);
		ImGui::DragFloat("PullBackRate", &_comp.pullBackRate, 0.1f, 0.0f, 60.0f);

		ImGui::Separator();
		ImGui::TextDisabled("At Full Speed");
		ImGui::DragFloat("FollowRateScale", &_comp.followRateScale, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("MaxLagAtSpeed", &_comp.maxLagAtSpeed, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("LookAtLagRatio", &_comp.lookAtLagRatio, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("FovAddAtSpeed", &_comp.fovAddAtSpeed, 0.5f, 0.0f, 90.0f);
		ImGui::DragFloat("FovRate", &_comp.fovRate, 0.1f, 0.0f, 60.0f);

		ImGui::Separator();
		ImGui::DragFloat("MaxLagDistance", &_comp.maxLagDistance, 0.1f, 0.0f, 100.0f);
	}
};
