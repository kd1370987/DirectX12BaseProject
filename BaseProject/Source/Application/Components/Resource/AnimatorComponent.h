#pragma once

struct AnimatorComponent
{
	uint32_t clipID = 0;
	Engine::Handle<Engine::Resource::AnimationData> animHandle;
	float time = 0.0f;
	float speed = 1.0f;

	Engine::ECS::Flg isLoop = 0;
};

template<>
struct Engine::ECS::ComponentTraits<AnimatorComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		AnimatorComponent& _comp = Engine::Editor::GetValue<AnimatorComponent>(a_pData);
		a_ar.Field("speed", _comp.speed);
		a_ar.Field("isLoop", _comp.isLoop);
	}

	static void Edit(void* a_pData)
	{
		AnimatorComponent& _comp = Engine::Editor::GetValue<AnimatorComponent>(a_pData);
		ImGui::Text("Handle : idx = %d,  gen = %d", (int)_comp.animHandle.GetIndex(), (int)_comp.animHandle.GetGeneration());
		ImGui::InputScalar("clipID", ImGuiDataType_U32, &_comp.clipID);
		ImGui::Text("Time : %f", &_comp.time);

		ImGui::DragFloat("Speed", &_comp.speed);

		ECS::Flg& _isLoop = _comp.isLoop;
		bool _value = _isLoop != 0;
		if (ImGui::Checkbox("IsLoop", &_value))
		{
			_isLoop = _value ? 1u : 0u;
		}
	}
};