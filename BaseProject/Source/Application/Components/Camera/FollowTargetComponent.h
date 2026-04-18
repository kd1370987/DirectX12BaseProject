#pragma once



struct FollowTargetComponent
{
	Engine::ECS::Entity target = Engine::ECS::Limits::INVALID_ENTITY;

	static constexpr auto GetFuncMeta()
	{
		using namespace Engine;
		return std::vector{
			Editor::CompEditFuncMeta{
				offsetof(FollowTargetComponent,target),
				[](void* a_data)
				{
					//ECS::Entity& _targetID = *reinterpret_cast<ECS::Entity*>(a_data);
					ImGui::InputScalar("TargetEntity",ImGuiDataType_U64,a_data);
				}
			}
		};
	};
};