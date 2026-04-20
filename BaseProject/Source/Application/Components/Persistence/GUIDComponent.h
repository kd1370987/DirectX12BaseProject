#pragma once
struct GUIDComponent
{
	UUID guid = {};

	static void Serialize(const void* a_ptr,nlohmann::json& a_json)
	{
		auto* _comp = static_cast<const GUIDComponent*>(a_ptr);
		a_json["guid"] = Engine::GUID::ToString(_comp->guid);
	}

	static void Deserialize(void* a_ptr,const nlohmann::json& a_json)
	{
		auto* _comp = static_cast<GUIDComponent*>(a_ptr);
		_comp->guid = Engine::GUID::FromString(a_json.at("guid"));
	}

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(GUIDComponent,guid),
				[](void* a_data)
				{
					// GUIDの表示のみ
					UUID& _guid = *reinterpret_cast<UUID*>(a_data);
					ImGui::Text("%s",Engine::GUID::ToString(_guid).c_str());
					//char* _guidStr = Engine::GUID::ToString(_guid).c_str;
					//ImGui::InputText("GUID",_guidStr, 64, ImGuiInputTextFlags_ReadOnly);
				}
			}
		};
	}
};