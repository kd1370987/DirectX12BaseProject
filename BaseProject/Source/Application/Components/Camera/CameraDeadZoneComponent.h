#pragma once

//==========================================================================================
// CameraDeadZoneComponent
//
// TPSカメラに付ける「自機を追いかける範囲」の設定。カメラ本体に付ける。
//
// 自機が画面のデッドゾーン(中央付近の矩形)の中に居る間、カメラは追いかけない。
// 枠から出たぶんだけカメラを平行移動させて、自機を枠の内側へ押し戻す。
//
// ・**カメラの向きは動かさない**。向きはマウス入力(LookAngle)だけで決まるので、
//   自機がどう動いても照準がブレない。追従は位置の平行移動だけで行う。
// ・デフォルトの構図(自機を画面のどこに置くか)を決めるのは注視点オフセット
//   (CameraFocusTargetComponent::offsetPos)。デッドゾーンはその位置を中心にして
//   「ここまでは外れてよい」という許容範囲を足すもの。
// ・枠の大きさは NDC(画面中心が0、端が1)の半径で持つ。
//     0.2 → 画面の左右中央 20% ぶんは自由に動ける
//     0.0 → 常に追従(デッドゾーン無し)
//   解像度が変わっても構図が変わらないよう、ピクセルではなく比率で持っている。
//==========================================================================================
struct CameraDeadZoneComponent
{
	// ---- 設定(保存される) ----

	// デッドゾーンの半径(NDC : 画面中心が 0、画面端が 1)
	Math::Vector2 halfExtents = { 0.22f, 0.16f };

	// 枠から出たときに押し戻す速さ(1秒あたりの指数減衰レート)。
	// 大きいほどすぐ枠へ戻る。0 以下なら追従しない
	float followRate = 8.0f;

	// 自機がカメラから離れすぎ/近づきすぎたときに詰める距離の許容(m)。
	// 画面の枠では前後方向を直せないので、奥行きだけ別に持つ。
	// 0 以下なら奥行きは常に合わせにいく
	float depthTolerance = 1.5f;

	// 奥行きを詰める速さ(1秒あたりの指数減衰レート)
	float depthFollowRate = 6.0f;

	// 追従を止める距離(m)。ここを超えたらデッドゾーンを無視して一気に寄せる。
	// テレポートやリスポーンでカメラが置き去りになるのを防ぐ保険。0 以下なら無効
	float snapDistance = 30.0f;

	// ---- ランタイム(保存しない。確認用) ----
	Math::Vector2 currentNdc = { 0.0f, 0.0f };	// いまの自機の画面位置(NDC)
	bool          isOutside  = false;			// 枠の外に出ているか

	// 枠の半径。負の値やゼロ除算になる値は入れさせない
	Math::Vector2 GetSafeHalfExtents() const
	{
		return {
			std::clamp(halfExtents.x, 0.0f, 1.0f),
			std::clamp(halfExtents.y, 0.0f, 1.0f)
		};
	}
};

template<>
struct Engine::ECS::ComponentTraits<CameraDeadZoneComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		CameraDeadZoneComponent& _comp = Engine::Editor::GetValue<CameraDeadZoneComponent>(a_pData);
		a_ar.Field("halfExtents",     _comp.halfExtents);
		a_ar.Field("followRate",      _comp.followRate);
		a_ar.Field("depthTolerance",  _comp.depthTolerance);
		a_ar.Field("depthFollowRate", _comp.depthFollowRate);
		a_ar.Field("snapDistance",    _comp.snapDistance);
	}

	static void Edit(CompEditContext& a_context)
	{
		CameraDeadZoneComponent& _comp = Engine::Editor::GetValue<CameraDeadZoneComponent>(a_context.pData);

		ImGui::Text("Dead Zone");
		ImGui::DragFloat2("HalfExtents (NDC)", &_comp.halfExtents.x, 0.01f, 0.0f, 1.0f);
		ImGui::TextDisabled("0 = 常に追従 / 1 = 画面端まで自由");
		ImGui::DragFloat("FollowRate", &_comp.followRate, 0.1f, 0.0f, 60.0f);

		ImGui::Separator();

		ImGui::Text("Depth");
		ImGui::DragFloat("DepthTolerance (m)", &_comp.depthTolerance, 0.1f, 0.0f);
		ImGui::DragFloat("DepthFollowRate", &_comp.depthFollowRate, 0.1f, 0.0f, 60.0f);

		ImGui::Separator();

		ImGui::DragFloat("SnapDistance (m)", &_comp.snapDistance, 0.5f, 0.0f);
		ImGui::TextDisabled("これ以上離れたら枠を無視して一気に寄せます");

		// 結果は毎フレーム上書きされるので表示のみ
		ImGui::Separator();
		ImGui::Text("ScreenNDC : %.2f, %.2f", _comp.currentNdc.x, _comp.currentNdc.y);
		ImGui::Text("Outside   : %s", _comp.isOutside ? "yes" : "no");
		ImGui::TextDisabled("既定の構図は CameraFocusTargetComponent の OffsetPos");
	}
};
