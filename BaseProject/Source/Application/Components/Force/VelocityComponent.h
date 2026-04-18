#pragma once

struct VelocityComponent
{
	DirectX::XMFLOAT3 value = { 0.0f, 0.0f, 0.0f };

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(VelocityComponent,value),
				[](void* a_data)
				{
					DirectX::XMFLOAT3& _value = Editor::GetValue<DirectX::XMFLOAT3>(a_data);
					ImGui::DragFloat3("Velocity",&_value.x);
				}
			}
		};
	};
};
