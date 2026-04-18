#pragma once

struct WorldMatrixComponent
{
	DirectX::XMFLOAT4X4 worldMat= {};

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(WorldMatrixComponent,worldMat),
				[](void* a_data)
				{
					float* _m = (float*)a_data;
					for (int _i = 0; _i < 4; ++_i)
					{
						ImGui::DragFloat4("##row",&_m[_i * 4]);
					}
				}
			}
		};
	};
};