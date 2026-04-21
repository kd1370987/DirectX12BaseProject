#pragma once

struct CameraParamComponent
{
	float fovY			= 60.0f;        // 垂直視野角(単位: 度)
	float aspectRatio	= 16.0f / 9.0f; // アスペクト比
	float nearZ			= 0.1f;			// ニアクリップ距離
	float farZ			= 1000.0f;	    // ファークリップ距離

	static void Serialize(const void* a_ptr, nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const CameraParamComponent*>(a_ptr);
		a_json["fovY"] = { _comp->fovY };
		a_json["aspectRatio"] = { _comp->aspectRatio };
		a_json["nearZ"] = { _comp->nearZ };
		a_json["farZ"] = { _comp->farZ };

	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<CameraParamComponent*>(a_ptr);
	}

	static void Edit(void* a_data)
	{
		CameraParamComponent& _comp = Engine::Editor::GetValue<CameraParamComponent>(a_data);
		ImGui::DragFloat("Fov", &_comp.fovY);
		ImGui::DragFloat("Aspect", &_comp.aspectRatio);
		ImGui::DragFloat("NearZ", &_comp.nearZ);
		ImGui::DragFloat("FarZ", &_comp.farZ);
	}
};