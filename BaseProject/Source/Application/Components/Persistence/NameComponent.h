#pragma once
struct NameComponent
{
	char name[64] = "Unknown";

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(NameComponent,name),
				[](void* a_data)
				{
				// 名前
				char* _name = reinterpret_cast<char*>(a_data);
				ImGui::InputText("##Name", _name, 64);
				ImGui::SameLine();
				ImGui::Text("Name");
			}
		}
		};
	}
};