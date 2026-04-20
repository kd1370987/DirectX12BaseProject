#pragma once

// 復元時はGUIDからの復元

struct HierarchyComponent
{
	// 親
	Engine::ECS::Entity parentID = Engine::ECS::Limits::INVALID_ENTITY;		// ランタイム用

	// 先頭のエンティティ
	Engine::ECS::Entity firstChildID = Engine::ECS::Limits::INVALID_ENTITY;

	// 自分より前のエンティティ
	Engine::ECS::Entity prevSiblingID = Engine::ECS::Limits::INVALID_ENTITY;

	// 自分の次に来るエンティティ
	Engine::ECS::Entity nextSiblingID = Engine::ECS::Limits::INVALID_ENTITY;

	// 先頭から自分が何階層目なのか
	// ソート時につかう
	UINT depth = 0;

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