#pragma once

struct SkeletonPoseComponent
{
	DirectX::XMFLOAT4X4 palette[300];

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(SkeletonPoseComponent,palette),
				[](void* a_data)
				{
					
				}
			}
		};
	}
};