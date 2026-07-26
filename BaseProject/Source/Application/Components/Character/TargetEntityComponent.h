struct TargetEntityComponent
{
	Engine::GUID targetGUID = Engine::DefaultGUID;
	Engine::ECS::Entity targetEntity = Engine::ECS::Limits::INVALID_ENTITY;

	bool isFind = false;
	float distance = 0.0f;
};

template<>
struct Engine::ECS::ComponentTraits<TargetEntityComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		TargetEntityComponent& _comp = Engine::Editor::GetValue<TargetEntityComponent>(a_pData);
		a_ar.Field("targetGUID",_comp.targetGUID);
	}

	static void Edit(CompEditContext& a_context)
	{
		TargetEntityComponent& _comp = Engine::Editor::GetValue<TargetEntityComponent>(a_context.pData);
		bool _isFind = _comp.isFind;
		ImGui::Checkbox("IsFind",&_isFind);
		ImGui::Text("Distance : %f",_comp.distance);

	}
};
