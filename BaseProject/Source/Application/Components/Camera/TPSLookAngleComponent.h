#pragma once

struct TPSLookAngleComponent
{
	float Pitch = 0.0f;
	float ClampPitch = 80.0f;

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(TPSLookAngleComponent,Pitch),
				[](void* a_data)
				{
					float& _value = Editor::GetValue<float>(a_data);
					ImGui::DragFloat("Pitch",&_value);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(TPSLookAngleComponent,ClampPitch),
				[](void* a_data)
				{
					float& _value = Editor::GetValue<float>(a_data);
					ImGui::DragFloat("ClampPitch",&_value);
				}
			}
		};
	};
};
