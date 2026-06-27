#pragma once

struct CameraParamComponent
{
	float fovY			= 60.0f;        // 垂直視野角(単位: 度)
	float aspectRatio	= 16.0f / 9.0f; // アスペクト比
	float nearZ			= 0.1f;			// ニアクリップ距離
	float farZ			= 1000.0f;	    // ファークリップ距離
};

template<>
struct Engine::ECS::ComponentTraits<CameraParamComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		CameraParamComponent& _comp = Engine::Editor::GetValue<CameraParamComponent>(a_pData);
		a_ar.Field("fovY",_comp.fovY);
		a_ar.Field("aspectRatio",_comp.aspectRatio);
		a_ar.Field("nearZ",_comp.nearZ);
		a_ar.Field("farZ",_comp.farZ);
	}

	static void Edit(void* a_pData)
	{
		CameraParamComponent& _comp = Engine::Editor::GetValue<CameraParamComponent>(a_pData);
		ImGui::DragFloat("Fov", &_comp.fovY);
		ImGui::DragFloat("Aspect", &_comp.aspectRatio);
		ImGui::DragFloat("NearZ", &_comp.nearZ);
		ImGui::DragFloat("FarZ", &_comp.farZ);
	}
};