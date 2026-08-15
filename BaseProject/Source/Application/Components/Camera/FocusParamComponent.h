#pragma once

// カメラのピント設定。被写界深度(DoF)の調整値をここで持つ。
//
//   ピントが合う範囲 : focusDistance ± focusRange * 0.5
//   そこから手前へ nearRange 進むと最大ボケ(CoC = -1)
//   そこから奥へ   farRange  進むと最大ボケ(CoC = +1)
//   最大ボケのときの半径が maxBlurRadius(ピクセル)
//
// アクティブカメラの値を CamSetShaderSystem が GraphicsEngine へ送り、
// CoCパス/DoFパスが定数バッファとして受け取る。
// (以前は全体設定の DoFOption が持っていたが、ピントはカメラの持ち物なので移した)
struct FocusParamComponent
{
	float focusDistance		= 1.0f;		// ピントが合う距離(カメラからの深度)
	float focusRange		= 0.1f;		// ピントが合う幅(この幅の中はボケない)
	float nearRange			= 0.1f;		// 手前側が最大ボケになるまでの距離
	float farRange			= 1000.0f;	// 奥側が最大ボケになるまでの距離
	float maxBlurRadius		= 8.0f;		// 最大ボケ半径(ピクセル)
	bool  enable			= false;	// false ならボカさずそのまま通す
};

template<>
struct Engine::ECS::ComponentTraits<FocusParamComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData) {
		auto* _comp = static_cast<FocusParamComponent*>(a_pData);
		a_ar.Field("focusDistance", _comp->focusDistance);
		a_ar.Field("focusRange", _comp->focusRange);
		a_ar.Field("nearRange", _comp->nearRange);
		a_ar.Field("farRange", _comp->farRange);
		a_ar.Field("maxBlurRadius", _comp->maxBlurRadius);
		a_ar.Field("enable", _comp->enable);
	}

	static void Edit(CompEditContext& a_context) {
		auto* _comp = static_cast<FocusParamComponent*>(a_context.pData);
		ImGui::Checkbox("DoF Enable", &_comp->enable);
		ImGui::DragFloat("FocusDistance", &_comp->focusDistance, 0.1f, 0.0f);
		ImGui::DragFloat("FocusRange", &_comp->focusRange, 0.1f, 0.0f);
		ImGui::DragFloat("NearRange", &_comp->nearRange, 0.1f, 0.0f);
		ImGui::DragFloat("FarRange", &_comp->farRange, 0.1f, 0.0f);
		ImGui::DragFloat("MaxBlurRadius", &_comp->maxBlurRadius, 0.1f, 0.0f, 32.0f);
	}
};
