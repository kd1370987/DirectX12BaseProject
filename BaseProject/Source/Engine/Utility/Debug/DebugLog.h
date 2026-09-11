#pragma once

namespace Engine::Debug
{
	// ログコールバック : エディターがあれば登録、なくてもVS側に出力
	using LogCallback = std::function<void(const char*)>;
	inline LogCallback g_logCallback = nullptr;

	inline void SetLogCallback(LogCallback a_cb) { g_logCallback = a_cb; }

	// =========================================================
	// 内部実装関数（直接呼ばず、マクロ経由で呼ぶ）
	// =========================================================
	namespace Internal
	{
		// UTF-8文字列を正しくVSの出力ウィンドウに送る関数
		inline void OutputDebugStringUTF8(const char* a_utf8Str)
		{
			// UTF-8 から UTF-16(ワイド文字) に変換するのに必要なサイズを計算
			int _size = MultiByteToWideChar(CP_UTF8, 0, a_utf8Str, -1, nullptr, 0);
			if (_size > 0) {
				std::wstring _wstr(_size, 0);
				MultiByteToWideChar(CP_UTF8, 0, a_utf8Str, -1, &_wstr[0], _size);
				// ワイド文字専用の OutputDebugStringW を使う
				OutputDebugStringW(_wstr.c_str());
			}
		}

		inline void LogImpl(const char* a_fmt, ...)
		{
			char _buf[2048];
			va_list _args;
			va_start(_args, a_fmt);
			vsnprintf(_buf, sizeof(_buf), a_fmt, _args);
			va_end(_args);


			std::string _str = _buf + std::string("\n");
			OutputDebugStringUTF8(_str.c_str());
			if (g_logCallback) g_logCallback(_str.c_str());
		}

		inline void WarningImpl(const char* a_fmt, ...)
		{
			char _buf[2048];
			va_list _args;
			va_start(_args, a_fmt);
			vsnprintf(_buf, sizeof(_buf), a_fmt, _args);
			va_end(_args);

			std::string _str = std::string("[WARNING] ") + _buf + std::string("\n");
			OutputDebugStringUTF8(_str.c_str());
			if (g_logCallback) g_logCallback(_str.c_str());
		}

		//----------------------------------------------------------------------
		// エラーとして記録するが、処理は止めない
		//
		// ErrLogImpl と違って assert しない。
		// 「失敗したことは残したいが、呼び出し側が false を返して続ける」
		// 種類の失敗(PSOの生成失敗・保存先が開けない など)はこちら。
		// 止めてよい失敗には ENGINE_ERRLOG(cond, ...) を使うこと
		//----------------------------------------------------------------------
		inline void ErrorImpl(const char* a_fmt, ...)
		{
			char _buf[2048];
			va_list _args;
			va_start(_args, a_fmt);
			vsnprintf(_buf, sizeof(_buf), a_fmt, _args);
			va_end(_args);

			std::string _str = std::string("[ERROR] ") + _buf + std::string("\n");
			OutputDebugStringUTF8(_str.c_str());
			if (g_logCallback) g_logCallback(_str.c_str());
		}

		inline void ErrLogImpl(bool a_cond, const char* a_fmt, ...)
		{
			if (a_cond) return;

			char _buf[2048];
			va_list _args;
			va_start(_args, a_fmt);
			vsnprintf(_buf, sizeof(_buf), a_fmt, _args);
			va_end(_args);

			std::string _str = std::string("[ERROR] ") + _buf + std::string("\n");
			OutputDebugStringUTF8(_str.c_str());
			if (g_logCallback) g_logCallback(_str.c_str());
			assert(false && "詳細はOutputDebugStringを確認");
		}
	}
}

// リリースビルド時でもログ出力してほしいフラグ
#define ENABLE_RELEASE_LOG

// =========================================================
// ログ用ヘルパーマクロ
// =========================================================
#define BOOL_STR(a_bool)((a_bool) ? "True" : "False")

// =========================================================
// ログ呼び出し用マクロ
// =========================================================
// _DEBUG または 独自の ENABLE_RELEASE_LOG が定義されている場合のみ有効化
#if defined(_DEBUG) || defined(DEBUG) || defined(ENABLE_RELEASE_LOG)
#define ENGINE_LOG(fmt, ...)          Engine::Debug::Internal::LogImpl(fmt, __VA_ARGS__)
#define ENGINE_WARNING(fmt, ...)      Engine::Debug::Internal::WarningImpl(fmt, __VA_ARGS__)
#define ENGINE_ERROR(fmt, ...)        Engine::Debug::Internal::ErrorImpl(fmt, __VA_ARGS__)
#define ENGINE_ERRLOG(cond, fmt, ...) Engine::Debug::Internal::ErrLogImpl(cond, fmt, __VA_ARGS__)
#else
	// 完全無効化（コストゼロ）
#define ENGINE_LOG(...)               __noop
#define ENGINE_WARNING(...)           __noop
#define ENGINE_ERROR(...)             __noop
#define ENGINE_ERRLOG(cond, ...)      __noop
#endif