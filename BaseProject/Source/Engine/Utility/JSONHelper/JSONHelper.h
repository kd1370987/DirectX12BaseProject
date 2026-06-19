#pragma once
namespace Engine::JSONHelper
{
	// JSONに値をセットする関数
	template<typename T>
	inline void SetValue(const std::string& a_key, nlohmann::json& a_json, const T& a_srcData)
	{
		// 型ごとに処理を特殊化
		if constexpr (std::is_same_v<T, DXSM::Vector2>)
		{
			a_json[a_key] = { a_srcData.x,a_srcData.y };
		}
		if constexpr (std::is_same_v<T, DirectX::XMFLOAT2>)
		{
			a_json[a_key] = { a_srcData.x,a_srcData.y };
		}
		else if constexpr (std::is_same_v<T, DXSM::Vector3>)
		{
			a_json[a_key] = { a_srcData.x,a_srcData.y,a_srcData.z };
		}
		else if constexpr (std::is_same_v<T, DirectX::XMFLOAT3>)
		{
			a_json[a_key] = { a_srcData.x,a_srcData.y,a_srcData.z };
		}
		else if constexpr (std::is_same_v<T, DXSM::Vector4>)
		{
			a_json[a_key] = { a_srcData.x,a_srcData.y,a_srcData.z,a_srcData.w };
		}
		else if constexpr (std::is_same_v<T, DXSM::Quaternion>)
		{
			a_json[a_key] = { a_srcData.x,a_srcData.y,a_srcData.z,a_srcData.w };
		}
		else if constexpr (std::is_same_v<T, DirectX::XMFLOAT4>)
		{
			a_json[a_key] = { a_srcData.x,a_srcData.y,a_srcData.z,a_srcData.w };
		}
		else if constexpr (std::is_same_v<T, DXSM::Matrix>)
		{
			a_json[a_key] = {
				a_srcData._11,a_srcData._12,a_srcData._13,a_srcData._14,
				a_srcData._21,a_srcData._22,a_srcData._23,a_srcData._24,
				a_srcData._31,a_srcData._32,a_srcData._33,a_srcData._34,
				a_srcData._41,a_srcData._42,a_srcData._43,a_srcData._44
			};
		}
		else if constexpr (std::is_same_v<T, DirectX::XMFLOAT4X4>)
		{
			a_json[a_key] = {
				a_srcData._11,a_srcData._12,a_srcData._13,a_srcData._14,
				a_srcData._21,a_srcData._22,a_srcData._23,a_srcData._24,
				a_srcData._31,a_srcData._32,a_srcData._33,a_srcData._34,
				a_srcData._41,a_srcData._42,a_srcData._43,a_srcData._44
			};
		}
		else if constexpr (std::is_same_v<T, Engine::GUID>)
		{
			// GUIDは文字列で初期化
			a_json[a_key] = a_srcData.String();
		}
		else
		{
			a_json[a_key] = a_srcData;
		}
	}

	// JSONから値を取得してくる関数
	template <typename T>
	inline T GetValue(const std::string& a_key, const nlohmann::json& a_json, const T& a_default)
	{
		// 文字列のキーを検索
		auto it = a_json.find(a_key);
		if (it == a_json.end() || it->is_null())
		{
			// 見つからなければデフォルト値を返す
			return a_default;
		}

		const auto& val = *it;

		// コンパイル時特殊化を使って特殊な型を処理する
		if constexpr (std::is_same_v<T, DXSM::Vector2>)
		{
			// 配列かつサイズ数が一定以上かどうか
			if (val.is_array() && val.size() >= 2 && val[0].is_number() && val[1].is_number())
			{
				return DXSM::Vector2{ val[0].get<float>(), val[1].get<float>() };
			}
			// 条件に合うデータでなければデフォルト値を返す
			return a_default;
		}
		if constexpr (std::is_same_v<T, DirectX::XMFLOAT2>)
		{
			// 配列かつサイズ数が一定以上かどうか
			if (val.is_array() && val.size() >= 2 && val[0].is_number() && val[1].is_number())
			{
				return DirectX::XMFLOAT2{ val[0].get<float>(), val[1].get<float>() };
			}
			// 条件に合うデータでなければデフォルト値を返す
			return a_default;
		}
		else if constexpr (std::is_same_v<T, DXSM::Vector3>)
		{
			if (val.is_array() && val.size() >= 3 && val[0].is_number() && val[1].is_number() && val[2].is_number())
			{
				return DXSM::Vector3{ val[0].get<float>(), val[1].get<float>(), val[2].get<float>() };
			}
			// 条件に合うデータでなければデフォルト値を返す
			return a_default;
		}
		else if constexpr (std::is_same_v<T, DirectX::XMFLOAT3>)
		{
			if (val.is_array() && val.size() >= 3 && val[0].is_number() && val[1].is_number() && val[2].is_number())
			{
				return DirectX::XMFLOAT3{ val[0].get<float>(), val[1].get<float>(), val[2].get<float>() };
			}
			// 条件に合うデータでなければデフォルト値を返す
			return a_default;
		}
		else if constexpr (std::is_same_v<T, DXSM::Vector4>)
		{
			if (val.is_array() && val.size() >= 4 && val[0].is_number() && val[1].is_number() && val[2].is_number() && val[3].is_number())
			{
				return DXSM::Vector4{ val[0].get<float>(), val[1].get<float>(), val[2].get<float>(), val[3].get<float>() };
			}
			// 条件に合うデータでなければデフォルト値を返す
			return a_default;
		}
		else if constexpr (std::is_same_v<T, DXSM::Quaternion>)
		{
			if (val.is_array() && val.size() >= 4 && val[0].is_number() && val[1].is_number() && val[2].is_number() && val[3].is_number())
			{
				return DXSM::Quaternion{ val[0].get<float>(), val[1].get<float>(), val[2].get<float>(), val[3].get<float>() };
			}
			// 条件に合うデータでなければデフォルト値を返す
			return a_default;
		}
		else if constexpr (std::is_same_v<T, DirectX::XMFLOAT4>)
		{
			if (val.is_array() && val.size() >= 4 && val[0].is_number() && val[1].is_number() && val[2].is_number() && val[3].is_number())
			{
				return DirectX::XMFLOAT4{ val[0].get<float>(), val[1].get<float>(), val[2].get<float>(), val[3].get<float>() };
			}
			// 条件に合うデータでなければデフォルト値を返す
			return a_default;
		}
		else if constexpr (std::is_same_v<T, DXSM::Matrix>)
		{

			if (val.is_array() && val.size() >= 16 &&
				val[0].is_number() && val[1].is_number() && val[2].is_number() && val[3].is_number() &&
				val[4].is_number() && val[5].is_number() && val[6].is_number() && val[7].is_number() &&
				val[8].is_number() && val[9].is_number() && val[10].is_number() && val[11].is_number() &&
				val[12].is_number() && val[13].is_number() && val[14].is_number() && val[15].is_number()
			)
			{
				return DXSM::Matrix{
					val[0].get<float>(), val[1].get<float>(), val[2].get<float>(), val[3].get<float>() ,
					val[4].get<float>(), val[5].get<float>(), val[6].get<float>(), val[7].get<float>() ,
					val[8].get<float>(), val[9].get<float>(), val[10].get<float>(), val[11].get<float>() ,
					val[12].get<float>(), val[13].get<float>(), val[14].get<float>(), val[15].get<float>() 
				};
			}
			// 条件に合うデータでなければデフォルト値を返す
			return a_default;
		}
		else if constexpr (std::is_same_v<T, DirectX::XMFLOAT4X4>)
		{

			if (val.is_array() && val.size() >= 16 &&
				val[0].is_number() && val[1].is_number() && val[2].is_number() && val[3].is_number() &&
				val[4].is_number() && val[5].is_number() && val[6].is_number() && val[7].is_number() &&
				val[8].is_number() && val[9].is_number() && val[10].is_number() && val[11].is_number() &&
				val[12].is_number() && val[13].is_number() && val[14].is_number() && val[15].is_number()
				)
			{
				return DirectX::XMFLOAT4X4{
					val[0].get<float>(), val[1].get<float>(), val[2].get<float>(), val[3].get<float>() ,
					val[4].get<float>(), val[5].get<float>(), val[6].get<float>(), val[7].get<float>() ,
					val[8].get<float>(), val[9].get<float>(), val[10].get<float>(), val[11].get<float>() ,
					val[12].get<float>(), val[13].get<float>(), val[14].get<float>(), val[15].get<float>()
				};
			}
			// 条件に合うデータでなければデフォルト値を返す
			return a_default;
		}
		else if constexpr (std::is_same_v<T, Engine::GUID>)
		{
			// GUIDは文字列で保存されている
			std::string _guidStr = "";
			if (val.is_string()) 
			{
				_guidStr = val.get<std::string>();
				Engine::GUID _guid;
				_guid.FromString(_guidStr);
				return _guid;
			}
			// 条件に合うデータでなければデフォルト値を返す
			return a_default;
		}
		// --- ここから下は汎用的な型の安全な取得 ---
		else if constexpr (std::is_floating_point_v<T> || std::is_integral_v<T>)
		{
			if (val.is_number()) return val.get<T>();
			// 条件に合うデータでなければデフォルト値を返す
			return a_default;
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			if (val.is_string()) return val.get<T>();
			// 条件に合うデータでなければデフォルト値を返す
			return a_default;
		}
		else if constexpr (std::is_same_v<T, bool>)
		{
			if (val.is_boolean()) return val.get<T>();
			// 条件に合うデータでなければデフォルト値を返す
			return a_default;
		}
		else
		{
			// どの型にも一致しなければ終了しないように
			// 一度valueで試したのちに、デフォルト値を返す
			try
			{
				return val.get<T>();
			}
			catch (...)
			{
				return a_default;
			}
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