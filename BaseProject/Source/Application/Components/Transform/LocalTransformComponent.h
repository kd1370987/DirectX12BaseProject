#pragma once

struct LocalTransformComponent
{
	DirectX::XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4 quat = { 0.0f, 0.0f, 0.0f,1.0f };
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	mutable bool isDirty = true;
};

template<>
struct Engine::ECS::ComponentTraits<LocalTransformComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		LocalTransformComponent& _comp = Engine::Editor::GetValue<LocalTransformComponent>(a_pData);
		a_ar.Field("pos",_comp.pos);
		a_ar.Field("quat",_comp.quat);
		a_ar.Field("scale",_comp.scale);

		_comp.isDirty = true;
	}
	static void Edit(CompEditContext& a_context)
	{
		LocalTransformComponent& _comp = Engine::Editor::GetValue<LocalTransformComponent>(a_context.pData);
		bool _isEdit = false;
		_isEdit |= ImGui::DragFloat3("Position", &_comp.pos.x);
		_isEdit |= Engine::Editor::Helper::DragRotationDeg3FromQuaternion(_comp.quat);
		_isEdit |= ImGui::DragFloat3("Scale", &_comp.scale.x);
		_comp.isDirty |= _isEdit;
	}
};