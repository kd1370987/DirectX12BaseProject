#pragma once
namespace StringUtility
{
	// std::wstring(ワイド文字列) -> std::string(マルチバイト文字列) 変換
	inline std::string ToUTF8(const std::wstring& a_value)
	{
		// バッファの確保
		auto _length = WideCharToMultiByte(CP_UTF8, 0U, a_value.data(), -1, nullptr, 0, nullptr, nullptr);
		auto _buffer = new char[_length];

		// 変換
		WideCharToMultiByte(CP_UTF8, 0U, a_value.data(), -1, _buffer, _length, nullptr, nullptr);

		// コピーして解放
		std::string _result(_buffer);
		delete[] _buffer;
		_buffer = nullptr;

		return _result;
	}

	// std::string(マルチバイト文字列) -> std::wstring(ワイド文字列) 変換
	inline std::wstring ToWideString(const std::string& a_str)
	{
		// マルチバイト解析
		auto _num1 = MultiByteToWideChar(
			CP_ACP, 0, a_str.c_str(), (int)a_str.size(), nullptr, 0);

		// ワイド文字列用意
		std::wstring _wstr;
		_wstr.resize(_num1);

		// 変換
		MultiByteToWideChar(CP_ACP, 0, a_str.c_str(), (int)a_str.size(), &_wstr[0], _num1);

		return _wstr;
	}

	// 文字列のハッシュ化
	inline UINT ToHash(const std::string& a_data)
	{
		// FNV-1a 32-bit
		const UINT _fnv_prime = 16777691u;
		const UINT _fnv_offset_basis = 2166136261u;

		UINT _hash = _fnv_offset_basis;
		for (char _c : a_data)
		{
			_hash ^= static_cast<UINT>(_c);
			_hash *= _fnv_prime;
		}
		return _hash;
	}
}