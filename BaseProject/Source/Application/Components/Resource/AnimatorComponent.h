#pragma once

enum class EAnimationState
{
	Idle,
	Walk,
	Jump,
	Fall,
	Boost,
	Landing,
	Damage,
	Dead,
};

struct AnimatorComponent
{
	EAnimationState animeState;
	uint32_t clipID = 0;
	Engine::Handle<Engine::Resource::AnimationData> animHandle;
	float time = 0.0f;
	float speed = 1.0f;

	Engine::ECS::Flg isLoop = 0;

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const AnimatorComponent*>(a_ptr);
		a_json["speed"] = _comp->speed;
		a_json["isLoop"] = static_cast<uint32_t>(_comp->isLoop);
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<AnimatorComponent*>(a_ptr);
		_comp->speed = a_json["speed"].get<float>();
		_comp->isLoop = a_json["isLoop"].get<Engine::ECS::Flg>();
	}

	static void Edit(void* a_data)
	{
		using namespace Engine;
		AnimatorComponent& _comp = Engine::Editor::GetValue<AnimatorComponent>(a_data);
		ImGui::Text("Handle : idx = %d,  gen = %d",(int)_comp.animHandle.GetIndex(), (int)_comp.animHandle.GetGeneration());
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