#pragma once

struct GravityComponent
{
	float scale = -1.0f;

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(GravityComponent,scale),
				[](void* a_data)
				{
					float& _value = Editor::GetValue<float>(a_data);
					ImGui::DragFloat("GravityScale",&_value,0.1f);
				}
			}
		};
	};
};