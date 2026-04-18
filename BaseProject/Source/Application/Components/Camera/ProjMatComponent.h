#pragma once

struct ProjMatComponent
{
	DirectX::XMFLOAT4X4 projMat = {};     // 射影行列
	DirectX::XMFLOAT4X4 projInvMat = {};  // 射影逆行列

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(ProjMatComponent,projMat),
				[](void* a_data)
				{
					float* m = (float*)a_data;
					for (int i = 0; i < 4; ++i)
					{
						ImGui::DragFloat4("##row", &m[i * 4]);
					}
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(ProjMatComponent,projInvMat),
				[](void* a_data)
				{
					float* m = (float*)a_data;
					for (int i = 0; i < 4; ++i)
					{
						ImGui::DragFloat4("##row", &m[i * 4]);
					}
				}
			}
		};
	};
};