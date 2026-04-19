#pragma once

constexpr UINT MAX_NODEINDEX = 100;

struct NodePoseComponent
{
	DirectX::XMFLOAT4X4 local[MAX_NODEINDEX];
	DirectX::XMFLOAT4X4 world[MAX_NODEINDEX];
	uint16_t nodeCount;

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(NodePoseComponent,local),
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
				offsetof(NodePoseComponent,world),
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
				offsetof(NodePoseComponent,nodeCount),
				[](void* a_data)
				{
					ImGui::InputScalar("NodeCount", ImGuiDataType_U16, a_data);
				}
			}
		};
	};
};