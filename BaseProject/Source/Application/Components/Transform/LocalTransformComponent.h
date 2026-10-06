#pragma once

namespace App::Component
{
	// 親子関係がある場合、親に影響される箇所
	// フラグが立っていれば、その箇所だけ親からの相対値になる
	enum class ETransformInheritance : uint8_t
	{
		None		= 0,
		Translation = 1 << 0,
		Rotation	= 1 << 1,
		Scale		= 1 << 2,
		All			= Translation | Rotation | Scale
	};
	ENUM_ATTR_BITFLAG(ETransformInheritance);		// BitFlag化

	struct LocalTransformComponent
	{
		ETransformInheritance inheritance = ETransformInheritance::All;

		Math::Vector3 pos = { 0.0f, 0.0f, 0.0f };
		Math::Quaternion quat = { 0.0f, 0.0f, 0.0f,1.0f };
		Math::Vector3 scale = { 1.0f, 1.0f, 1.0f };

		mutable bool isDirty = true;
	};
}

template<>
struct Engine::ECS::ComponentTraits<App::Component::LocalTransformComponent>
{
	static void Archive(Engine::Persistence::Archive& a_ar, void* a_pData)
	{
		App::Component::LocalTransformComponent& _comp = Engine::EditorField::GetValue<App::Component::LocalTransformComponent>(a_pData);
		a_ar.Field("pos",_comp.pos);
		a_ar.Field("quat",_comp.quat);
		a_ar.Field("scale",_comp.scale);
		a_ar.Field("inheritance",_comp.inheritance);

		_comp.isDirty = true;
	}
	static void Edit(CompEditContext& a_context)
	{
		App::Component::LocalTransformComponent& _comp = Engine::EditorField::GetValue<App::Component::LocalTransformComponent>(a_context.pData);

		Engine::EditorField::FlagsField("ETransformInheritance",_comp.inheritance);

		bool _isEdit = false;
		_isEdit |= Engine::EditorField::Field("Position", _comp.pos);
		_isEdit |= Engine::EditorField::Field("Rotation", _comp.quat);
		_isEdit |= Engine::EditorField::Field("Scale", _comp.scale);
		_comp.isDirty |= _isEdit;
	}
};