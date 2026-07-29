#pragma once

// TPSカメラの実行時状態(TPSSystem が毎フレーム更新する)
struct TPSCameraStateComponent
{
	DirectX::XMFLOAT3 currentLookAt = { 0.0f, 0.0f, 0.0f }; // 現在カメラが見つめている実際の座標
	DirectX::XMFLOAT4 currentOrbit  = { 0.0f, 0.0f, 0.0f, 1.0f }; // 現在のオービット回転(Slerp補間用)

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
		ImGui::Text("CurrentLookAt : %.2f, %.2f, %.2f",
			_comp.currentLookAt.x, _comp.currentLookAt.y, _comp.currentLookAt.z);
		ImGui::Text("CurrentPivot  : %.2f, %.2f, %.2f",
			_comp.currentPivot.x, _comp.currentPivot.y, _comp.currentPivot.z);
		ImGui::Text("PullBack      : %.2f", _comp.currentPullBack);
	}
};
