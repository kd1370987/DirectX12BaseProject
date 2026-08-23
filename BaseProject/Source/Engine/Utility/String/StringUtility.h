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

	/// <summary>UTF-8文字列をコードポイントの並びへ分解する</summary>
	/// <remarks>
	/// フォントは「1文字 = 1コードポイント」で引くので、
	/// バイト列のままでは日本語のような多バイト文字を1文字として扱えない。
	/// ソースは /utf-8 でコンパイルしているため、C++の文字列リテラルはそのまま渡してよい。
	///
	/// 壊れたバイトが来たら U+FFFD (置換文字) に潰して1バイト進める。
	/// 読めない文字で止まると、そこから先が全部出なくなるため
	/// </remarks>
	inline std::vector<uint32_t> ToCodePoints(std::string_view a_utf8)
	{
		constexpr uint32_t _REPLACEMENT = 0xFFFD;	// 壊れたバイトの代わりに使う文字

		std::vector<uint32_t> _result = {};
		_result.reserve(a_utf8.size());

		const size_t _size = a_utf8.size();
		for (size_t _i = 0; _i < _size; )
		{
			const uint8_t _lead = static_cast<uint8_t>(a_utf8[_i]);

			// 何バイトの文字かを先頭バイトから決める
			uint32_t _codePoint = 0;
			size_t _length = 0;
			if (_lead < 0x80) { _codePoint = _lead;			_length = 1; }
			else if ((_lead & 0xE0) == 0xC0) { _codePoint = _lead & 0x1Fu;	_length = 2; }
			else if ((_lead & 0xF0) == 0xE0) { _codePoint = _lead & 0x0Fu;	_length = 3; }
			else if ((_lead & 0xF8) == 0xF0) { _codePoint = _lead & 0x07u;	_length = 4; }
			else
			{
				// 先頭バイトとして有り得ない値
				_result.push_back(_REPLACEMENT);
				++_i;
				continue;
			}

			// 続きのバイトが足りない
			if (_i + _length > _size)
			{
				_result.push_back(_REPLACEMENT);
				++_i;
				continue;
			}

			// 続きのバイトを繋げる
			bool _isValid = true;
			for (size_t _j = 1; _j < _length; ++_j)
			{
				const uint8_t _next = static_cast<uint8_t>(a_utf8[_i + _j]);
				if ((_next & 0xC0) != 0x80) { _isValid = false; break; }
				_codePoint = (_codePoint << 6) | (_next & 0x3Fu);
			}

			if (!_isValid)
			{
				_result.push_back(_REPLACEMENT);
				++_i;
				continue;
			}

			_result.push_back(_codePoint);
			_i += _length;
		}

		return _result;
	}
}
