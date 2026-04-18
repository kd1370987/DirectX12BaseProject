#pragma once
struct GUIDComponent
{
	UUID guid = {};

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