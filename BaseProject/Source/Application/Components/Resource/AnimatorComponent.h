#pragma once

struct AnimatorComponent
{
	uint32_t clipID = 0;
	float time = 0.0f;
	float speed = 1.0f;

	Engine::ECS::Flg isLoop = 0;

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(AnimatorComponent,clipID),
				[](void* a_data)
				{
					ImGui::InputScalar("ClipID",ImGuiDataType_U32,a_data);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(AnimatorComponent,time),
				[](void* a_data)
				{
					float& _time = *reinterpret_cast<float*>(a_data);
					ImGui::DragFloat("Time",&_time);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(AnimatorComponent,speed),
				[](void* a_data)
				{
					float& _speed = *reinterpret_cast<float*>(a_data);
					ImGui::DragFloat("Speed",&_speed);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(AnimatorComponent,isLoop),
				[](void* a_data)
				{
					ECS::Flg& _isLoop = *reinterpret_cast<ECS::Flg*>(a_data);
					bool _value = _isLoop != 0;
					if(ImGui::Checkbox("IsLoop",&_value))
					{
						_isLoop = _value ? 1u : 0u;
					}
				}
			},
		};
	}
};