#pragma once

struct CameraParamComponent
{
	float fovY			= 60.0f;        // 垂直視野角(単位: 度)
	float aspectRatio	= 16.0f / 9.0f; // アスペクト比
	float nearZ			= 0.1f;			// ニアクリップ距離
	float farZ			= 1000.0f;	    // ファークリップ距離

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(CameraParamComponent,fovY),
				[](void* a_data)
				{
					float& _fovY = Editor::GetValue<float>(a_data);
					ImGui::DragFloat("FovY",&_fovY);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(CameraParamComponent,aspectRatio),
				[](void* a_data)
				{
					float& _asp = Editor::GetValue<float>(a_data);
					ImGui::DragFloat("AspectRatio",&_asp);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(CameraParamComponent,nearZ),
				[](void* a_data)
				{
					// スケール
					float& _near = Editor::GetValue<float>(a_data);
					ImGui::DragFloat("Near",&_near);
				}
			},
			Editor::CompEditFuncMeta{
				offsetof(CameraParamComponent,farZ),
				[](void* a_data)
				{
				// スケール
				float& _farZ = Editor::GetValue<float>(a_data);
				ImGui::DragFloat("Far",&_farZ);
			}
		}
		};
	};
};