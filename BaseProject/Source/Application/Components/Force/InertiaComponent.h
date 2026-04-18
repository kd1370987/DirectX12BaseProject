#pragma once

// 慣性コンポーネント

struct InertiaComponent
{
	float value = 0.0f;

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(InertiaComponent,value),
				[](void* a_data)
				{
					DirectX::XMFLOAT3& _value = Editor::GetValue<DirectX::XMFLOAT3>(a_data);
					ImGui::DragFloat("InertiaScale",&_value.x,0.1f);
				}
			}
		};
	};
};