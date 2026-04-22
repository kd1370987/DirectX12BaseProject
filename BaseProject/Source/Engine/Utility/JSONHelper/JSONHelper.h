#pragma once
namespace Engine::JSONHelper
{
	// 変数取得
	template <typename T>
	inline T GetValue(const std::string& a_key, const nlohmann::json & a_json,const T& a_default)
	{
		if (!a_json.contains(a_key))
		{
			return a_default;
		}
		else
		{
			return a_json[a_key].get<T>();
		}
	}

	// ベクター２用
	inline DXSM::Vector2 GetVec2(const std::string& a_key, const nlohmann::json& a_json, DXSM::Vector2 a_default)
	{
		if (!a_json.contains(a_key))
			return a_default;

		const auto& value = a_json.at(a_key);

		if (!value.is_array() || value.size() < 2)
			return a_default;

		try
		{
			return DXSM::Vector2{
				value[0].get<float>(),
				value[1].get<float>()
			};
		}
		catch (...)
		{
			return a_default;
		}
	}

	// ベクター3用
	inline DXSM::Vector3 GetVec3(const std::string& a_key, const nlohmann::json& a_json, DXSM::Vector3 a_default)
	{
		if (!a_json.contains(a_key))
			return a_default;

		const auto& value = a_json.at(a_key);

		if (!value.is_array() || value.size() < 3)
			return a_default;

		try
		{
			return DXSM::Vector3{
				value[0].get<float>(),
				value[1].get<float>(),
				value[2].get<float>()
			};
		}
		catch (...)
		{
			return a_default;
		}
	}

	// ベクター4用
	inline DXSM::Vector4 GetVec4(const std::string& a_key, const nlohmann::json& a_json, DXSM::Vector4 a_default)
	{
		if (!a_json.contains(a_key))
			return a_default;

		const auto& value = a_json.at(a_key);

		if (!value.is_array() || value.size() < 4)
			return a_default;

		try
		{
			return DXSM::Vector4{
				value[0].get<float>(),
				value[1].get<float>(),
				value[2].get<float>(),
				value[3].get<float>()
			};
		}
		catch (...)
		{
			return a_default;
		}
	}
}