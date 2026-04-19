#pragma once

struct TRSComponent
{
	DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 quat = { 0.0f, 0.0f, 0.0f,1.0f };
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(TRSComponent,pos),
				[](void* a_data)
				{
					// ポジション
					DirectX::XMFLOAT3& _pos = *reinterpret_cast<DirectX::XMFLOAT3*>(a_data);
					ImGui::DragFloat3("Position",&_pos.x);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(TRSComponent,quat),
				[](void* a_data)
				{
					// クォータニオンは変換して操作
					DirectX::XMFLOAT4& _quat = *reinterpret_cast<DirectX::XMFLOAT4*>(a_data);
					ImGui::DragFloat4("Quaternion",&_quat.x);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(TRSComponent,scale),
				[](void* a_data)
				{
					// スケール
					DirectX::XMFLOAT3& _scale = *reinterpret_cast<DirectX::XMFLOAT3*>(a_data);
					ImGui::DragFloat3("Scale",&_scale.x);
				}
			}
		};
	};
};