#pragma once

// TPSカメラの実行時状態(TPSSystem が毎フレーム更新する)
struct TPSCameraStateComponent
{
	// 現在の注視点。ワールドの絶対座標ではなく「カメラ空間(オービット基準)の相対座標」で持つ。
	// 左手系なので +Z が視線の奥、+X が画面右、+Y が画面上。
	// 機体の姿勢で解決すると、胴体が視線と別方向を向いた瞬間に構図が振られる。
	DirectX::XMFLOAT3 currentLookAt = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 currentOrbit  = { 0.0f, 0.0f, 0.0f, 1.0f }; // 現在のオービット回転(Slerp補間用)

	// currentLookAt をワールドへ解決した結果。TPSSystem が毎フレーム書く。
	// 他システム(AimTargetSystem)がオービットの式を再現しなくて済むように置いている。
	// 実行時の派生値なので保存しない。
	DirectX::XMFLOAT3 lookAtWorld = { 0.0f, 0.0f, 0.0f };

	DirectX::XMFLOAT3 currentPivot  = { 0.0f, 0.0f, 0.0f }; // 現在のピボット。ターゲットへ遅れて追従する
	float currentPullBack = 0.0f;							// 現在の引き量(m)。水平速度に応じて伸びる

	// 追従状態が一度でも作られたか。
	// 保存しないので、シーンをロードした直後の1フレーム目は必ずターゲットへスナップする
	// (原点から飛んでくるのを防ぐため)。
	bool isInitialized = false;
};

template<>
struct Engine::ECS::ComponentTraits<TPSCameraStateComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		TPSCameraStateComponent& _comp = Engine::Editor::GetValue<TPSCameraStateComponent>(a_pData);
		a_ar.Field("currentLookAt", _comp.currentLookAt);
		a_ar.Field("currentOrbit", _comp.currentOrbit);
	}

	static void Edit(CompEditContext& a_context)
	{
		TPSCameraStateComponent& _comp = Engine::Editor::GetValue<TPSCameraStateComponent>(a_context.pData);

		// システムが毎フレーム上書きするので表示のみ
		ImGui::Text("LookAtCamera  : %.2f, %.2f, %.2f",
			_comp.currentLookAt.x, _comp.currentLookAt.y, _comp.currentLookAt.z);
		ImGui::Text("LookAtWorld   : %.2f, %.2f, %.2f",
			_comp.lookAtWorld.x, _comp.lookAtWorld.y, _comp.lookAtWorld.z);
		ImGui::Text("CurrentPivot  : %.2f, %.2f, %.2f",
			_comp.currentPivot.x, _comp.currentPivot.y, _comp.currentPivot.z);
		ImGui::Text("PullBack      : %.2f", _comp.currentPullBack);
	}
};
