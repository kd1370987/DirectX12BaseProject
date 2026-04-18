#pragma once

struct PlayerLookAngleComponent
{
	float Yaw = 0.0f;

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(PlayerLookAngleComponent,Yaw),
				[](void* a_data)
				{
					float& _value = Editor::GetValue<float>(a_data);
					ImGui::DragFloat("Yaw",&_value,0.1f);
				}
			}
		};
	};

};