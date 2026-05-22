#include "SystemView.h"

#include "Application/Scene/SceneManager.h"
#include "Engine/ECS/World/World.h"

namespace Engine::Editor
{
	void SystemView::Init()
	{}

	void SystemView::Draw(UINT a_width, UINT a_height)
	{
		Engine::ECS::World* _pWorld = SceneManager::Instance().RefWorld();
		if (!_pWorld || !_pWorld->IsInit())
			return;

		if (ImGui::Begin("Systems Viewer", nullptr, ImGuiWindowFlags_MenuBar))
		{
			const auto& _compileSystemMap = _pWorld->GetCompileTaskMap();

			ImGui::Text("SystemType Count : %d",
				static_cast<int>(_compileSystemMap.size())
			);

			ImGui::Separator();

			for (auto& [_type, _taskVec] : _compileSystemMap)
			{
				auto _name = magic_enum::enum_name(_type);

				// グループタイトル
				if (ImGui::TreeNodeEx(
					(std::string(_name) + "##group").c_str(),
					ImGuiTreeNodeFlags_Selected,
					"%.*s (%d)",
					static_cast<int>(_name.size()),
					_name.data(),
					static_cast<int>(_taskVec.size())
				))
				{
					if (ImGui::BeginTable(
						(std::string("Table") + std::string(_name)).c_str(),
						3,
						ImGuiTableFlags_RowBg |
						ImGuiTableFlags_Borders |
						ImGuiTableFlags_Resizable |
						ImGuiTableFlags_SizingStretchProp))
					{
						ImGui::TableSetupColumn("Index");
						ImGui::TableSetupColumn("System Name");
						ImGui::TableSetupColumn("Enabled");
						ImGui::TableHeadersRow();

						for (size_t i = 0; i < _taskVec.size(); ++i)
						{
							auto* _task = _taskVec[i];

							ImGui::TableNextRow();

							// Index
							ImGui::TableSetColumnIndex(0);
							ImGui::Text("%d", static_cast<int>(i));

							// Name
							ImGui::TableSetColumnIndex(1);

							ImGui::Selectable(
								_task->name.c_str(),
								false,
								ImGuiSelectableFlags_SpanAllColumns);

							// Enabled (将来用)
							ImGui::TableSetColumnIndex(2);

							bool _enabled = true;
							ImGui::Checkbox(
								(std::string("##") + _task->name).c_str(),
								&_enabled);
						}

						ImGui::EndTable();
					}

					ImGui::TreePop();
				}
			}
		}

		ImGui::End();
	}
}