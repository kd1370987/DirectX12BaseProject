#pragma once
namespace Engine::ECS
{
	class World;
}

struct FielMeta
{
	const char* name;
	size_t offset;

	enum class Type
	{
		Bool,
		U32,
		U64,
		Float,
		Float2,
		Float3,
		Float4,
		Matrix,
		Text,
		Enum,
		EnumFlag,
	} type;
};

using EditFunc = std::function<void(const Engine::ECS::Entity&)>;

class ComponentEdit
{
public:

	void Init();

	void Register(Engine::ECS::World* a_pWorld, Engine::ECS::ComponentTypeID a_typeID,const std::vector<FielMeta>& a_data);

	
	EditFunc GetCompEditFunc(Engine::ECS::World* a_pWorld, Engine::ECS::ComponentTypeID a_typeID);

	template<typename E>
	void DrawEnumCombo(const char* a_lable, E& a_value);

	template<typename E>
	void DrawEnumFlagsCombo(const char* a_lable, E& a_value);

private:

	EditFunc m_defaultFunc;

	std::unordered_map<Engine::ECS::ComponentTypeID, EditFunc> m_editFuncMap = {};


};

template<typename E>
inline void ComponentEdit::DrawEnumCombo(const char* a_lable, E& a_value)
{
	const char* _preview = magic_enum::enum_name(a_value).data();

	if (ImGui::BeginCombo(a_lable, _preview))
	{
		for (auto _v : magic_enum::enum_values<E>())
		{
			bool _isSelect = (_v == a_value);

			if (ImGui::Selectable(magic_enum::enum_name(_v).data(), _isSelect))
			{
				a_value = _v;
			}

			if (_isSelect)
			{
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}
}

template<typename E>
inline void ComponentEdit::DrawEnumFlagsCombo(const char* a_lable, E& a_value)
{
	using U = std::underlying_type_t<E>;
	U _raw = static_cast<U>(a_value);

	std::string _preview = "None";

	if (_raw != 0)
	{
		_preview.clear();
		for (auto _v : magic_enum::enum_values<E>())
		{
			U _bit = static_cast<U>(_v);
			if (_bit != 0 && (_raw & _bit))
			{
				if (!_preview.empty())
				{
					_preview += " | ";
				}
				_preview += magic_enum::enum_name(_v);
			}
		}
	}

	if (ImGui::BeginCombo(a_lable, _preview.c_str()))
	{
		for (auto _v : magic_enum::enum_values<E>())
		{
			if (_v == E{}) continue;		// None除外するなら

			U _bit = static_cast<U>(_v);
			bool _isCheck = (_raw & _bit) != 0;

			if (ImGui::Selectable(magic_enum::enum_name(_v).data(), _isCheck))
			{
				if (_isCheck)
				{
					_raw &= ~_bit;
				}
				else
				{
					_raw |= _bit;
				}
			}
		}
		a_value = static_cast<E>(_raw);
		ImGui::EndCombo();
	}
}
