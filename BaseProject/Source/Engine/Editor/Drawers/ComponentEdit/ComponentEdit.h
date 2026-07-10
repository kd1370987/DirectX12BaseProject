#pragma once

namespace Engine::Editor
{
	// 型安全に値を参照する
	template<typename T>
	T& GetValue(void* a_data)
	{
		return *reinterpret_cast<T*>(a_data);
	}

	// Enumをコンボとして描画するヘルパー関数
	template<typename Enum>
	void DrawEnumCombo(const char* a_lable, Enum& a_value)
	{
		const char* _preview = magic_enum::enum_name(a_value).data();

		if (ImGui::BeginCombo(a_lable, _preview))
		{
			for (auto _v : magic_enum::enum_values<Enum>())
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
	// Enumをビットのコンボとして描画するヘルパー関数
	template<typename Enum>
	void DrawEnumFlagsCombo(const char* a_lable, Enum& a_value)
	{
		using U = std::underlying_type_t<Enum>;
		U _raw = static_cast<U>(a_value);

		std::string _preview = "None";

		if (_raw != 0)
		{
			_preview.clear();
			for (auto _v : magic_enum::enum_values<Enum>())
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
			for (auto _v : magic_enum::enum_values<Enum>())
			{
				if (_v == Enum{}) continue;		// None除外するなら

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
			a_value = static_cast<Enum>(_raw);
			ImGui::EndCombo();
		}
	}
}