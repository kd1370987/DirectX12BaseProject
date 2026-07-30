#pragma once

// 慣性コンポーネント

struct InertiaComponent
{
	// 慣性の強さ(目標速度へ追従しきるまでの時定数:秒)
	// 0.0f なら慣性なしで、速度がそのまま移動に反映される
	float value = 0.0f;

	// 慣性を適用したあとの現在速度(実行時のみ使用)
	DirectX::XMFLOAT3 velocity = { 0.0f, 0.0f, 0.0f };
};

template<>
struct Engine::ECS::ComponentTraits<InertiaComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		InertiaComponent& _comp = Engine::Editor::GetValue<InertiaComponent>(a_pData);
		a_ar.Field("Inertia", _comp.value);
	}

	static void Edit(CompEditContext& a_context)
	{
		InertiaComponent& _comp = Engine::Editor::GetValue<InertiaComponent>(a_context.pData);
		ImGui::DragFloat("Inertia", &_comp.value, 0.1f, 0.0f, FLT_MAX);
		ImGui::LabelText("InertiaVelocity", "%.2f, %.2f , %.2f", _comp.velocity.x, _comp.velocity.y, _comp.velocity.z);
	}
};