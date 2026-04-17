#pragma once

struct CameraParamComponent
{
	float fovY			= 60.0f;        // 垂直視野角(単位: 度)
	float aspectRatio	= 16.0f / 9.0f; // アスペクト比
	float nearZ			= 0.1f;			// ニアクリップ距離
	float farZ			= 1000.0f;	    // ファークリップ距離

	static constexpr auto GetMeta()
	{
		return std::vector{
			Engine::Editor::FielMeta{"FovY", offsetof(CameraParamComponent, fovY), Engine::Editor::FielMeta::Type::Float},
			Engine::Editor::FielMeta{"AspectRate", offsetof(CameraParamComponent, aspectRatio), Engine::Editor::FielMeta::Type::Float},
			Engine::Editor::FielMeta{"NearClip", offsetof(CameraParamComponent, nearZ), Engine::Editor::FielMeta::Type::Float},
			Engine::Editor::FielMeta{"FarClip", offsetof(CameraParamComponent, farZ), Engine::Editor::FielMeta::Type::Float}
		};
	}
};