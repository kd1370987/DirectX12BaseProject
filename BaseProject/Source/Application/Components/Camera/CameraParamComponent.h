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
		a_json["fovY"] = _comp->fovY;
		a_json["aspectRatio"] = _comp->aspectRatio;
		a_json["nearZ"] = _comp->nearZ;
		a_json["farZ"] = _comp->farZ;
	}

	static void Deserialize(void* a_ptr, const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<CameraParamComponent*>(a_ptr);
		_comp->fovY = Engine::JSONHelper::GetValue<float>("fovY",a_json,90.0f);
		_comp->aspectRatio = Engine::JSONHelper::GetValue<float>("aspectRatio", a_json, 16.f);
		_comp->nearZ = Engine::JSONHelper::GetValue<float>("nearZ", a_json, 0.1f); 
		_comp->farZ = Engine::JSONHelper::GetValue<float>("farZ", a_json, 1000.f); 
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