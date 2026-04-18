#pragma once

struct FocusParamComponent
{
	float focusDistance		= 0.0f;  // 焦点距離
	float forcusRange		= 0.0f;   // 焦点範囲
	float forcusBackRange	= 1000.0f; // 焦点後ろ範囲

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(FocusParamComponent,focusDistance),
				[](void* a_data)
				{
					float& _distance = Editor::GetValue<float>(a_data);
					ImGui::DragFloat("ForcusDistance",&_distance);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(FocusParamComponent,forcusRange),
				[](void* a_data)
				{
					float& _range = Editor::GetValue<float>(a_data);
					ImGui::DragFloat("ForcusRange",&_range);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(FocusParamComponent,forcusBackRange),
				[](void* a_data)
				{
					float& _backRange = Editor::GetValue<float>(a_data);
					ImGui::DragFloat("ForcusBackRange",&_backRange);
				}
			}
		};
	};
};