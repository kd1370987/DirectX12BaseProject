#pragma once

// 復元時はGUIDからの復元

struct MoveIntentComponent
{
	DirectX::XMFLOAT3 value;
	float jumpPow = 0.0f;
};

template<>
struct Engine::ECS::ComponentTraits<MoveIntentComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		MoveIntentComponent& _comp = Engine::Editor::GetValue<MoveIntentComponent>(a_pData);
		a_ar.Field("jumpPow", _comp.jumpPow);
	}

	static void Edit(CompEditContext& a_context)
	{
		MoveIntentComponent& _comp = Engine::Editor::GetValue<MoveIntentComponent>(a_context.pData);
		ImGui::Text("MoveIntent");
		ImGui::Text("x : %f", _comp.value.x);
		ImGui::Text("y : %f", _comp.value.y);
		ImGui::Text("z : %f", _comp.value.z);

		ImGui::Separator();

		ImGui::DragFloat("JumpPow", &_comp.jumpPow, 0.01f, 0);
	}
};