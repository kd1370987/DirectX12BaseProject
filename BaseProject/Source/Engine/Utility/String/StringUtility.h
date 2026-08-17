#pragma once

//==========================================================================================
// Engine::String
//
// 文字列の変換とハッシュ化(以前の Core/Utility/StringUtility.h)。
//==========================================================================================
namespace Engine::String
{
	/// <summary>std::wstring(ワイド文字列) -> std::string(UTF-8) 変換</summary>
	inline std::string ToUTF8(const std::wstring& a_value)
	{
		if (a_value.empty()) return {};

		// 必要なバイト数を先に測る(終端の分を含む)
		const int _length = WideCharToMultiByte(
			CP_UTF8, 0U, a_value.data(), -1, nullptr, 0, nullptr, nullptr);
		if (_length <= 0) return {};

		// 終端は std::string 側が持つので、その分を引いた長さで確保する
		std::string _result(static_cast<size_t>(_length) - 1, '\0');
		WideCharToMultiByte(
			CP_UTF8, 0U, a_value.data(), -1, _result.data(), _length, nullptr, nullptr);

		return _result;
	}

	/// <summary>std::string(マルチバイト文字列) -> std::wstring(ワイド文字列) 変換</summary>
	inline std::wstring ToWideString(const std::string& a_str)
	{
		if (a_str.empty()) return {};

		// マルチバイト解析
		const int _length = MultiByteToWideChar(
			CP_ACP, 0, a_str.c_str(), static_cast<int>(a_str.size()), nullptr, 0);
		if (_length <= 0) return {};

		// ワイド文字列用意
		std::wstring _wstr(static_cast<size_t>(_length), L'\0');

		// 変換
		MultiByteToWideChar(
			CP_ACP, 0, a_str.c_str(), static_cast<int>(a_str.size()), _wstr.data(), _length);

		return _wstr;
	}

	/// <summary>文字列のハッシュ化(FNV-1a 32bit)</summary>
	/// <remarks>
	/// 名前をIDとして持ちたい所で使う。実行ごとに値は変わらないので保存しても良いが、
	/// 衝突しない保証は無いので、突き合わせに使うなら元の文字列も持っておくこと。
	/// </remarks>
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
