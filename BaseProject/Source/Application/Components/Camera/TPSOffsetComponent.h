#pragma once

struct TPSOffsetComponent
{
	float x = 0, y = 0, z = 0;

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(TPSOffsetComponent,x),
				[](void* a_data)
				{
					float& _value = Editor::GetValue<float>(a_data);
					ImGui::DragFloat("X",&_value);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(TPSOffsetComponent,y),
				[](void* a_data)
				{
					float& _value = Editor::GetValue<float>(a_data);
					ImGui::DragFloat("Y",&_value);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(TPSOffsetComponent,z),
				[](void* a_data)
				{
					float& _value = Editor::GetValue<float>(a_data);
					ImGui::DragFloat("Z",&_value);
				}
			}
		};
	};

};