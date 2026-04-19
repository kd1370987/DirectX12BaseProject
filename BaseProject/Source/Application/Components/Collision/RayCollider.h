#pragma once

struct RayColliderComponent
{
	float length = 0.0f;
	DirectX::XMFLOAT3 dir = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT3 pos = { 0.0f,0.0f,0.0f };

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(RayColliderComponent,length),
				[](void* a_data)
				{
					float& _length = Editor::GetValue<float>(a_data);
					ImGui::DragFloat("Length", &_length, 0.1f);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(RayColliderComponent,dir),
				[](void* a_data)
				{
					DirectX::XMFLOAT3& _dir = Editor::GetValue<DirectX::XMFLOAT3>(a_data);
					ImGui::DragFloat3("Dir", &_dir.x, 0.1f);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(RayColliderComponent,pos),
				[](void* a_data)
				{
					DirectX::XMFLOAT3& _pos = Editor::GetValue<DirectX::XMFLOAT3>(a_data);
					ImGui::DragFloat3("Pos", &_pos.x, 0.1f);
				}
			}
		};
	};
};