#pragma once

namespace Engine
{
	struct GUID
	{
		UUID value;

		// 新たに作成
		void Create();

		// 文字列へ変換
		std::string String() const;

		// 文字列からの再生
		void FromString(const std::string& a_str);

		// ハッシュの取得
		size_t Hash() const noexcept;

		// 宇宙船が定義されていなさそうだから
		// 自分でオペレーター定義
		bool operator==(const GUID& other) const
		{
			return InlineIsEqualGUID(value,other.value);
		}
	};
}

namespace std
{
	template<>
	struct hash<Engine::GUID>
	{
		size_t operator()(const Engine::GUID& g) const noexcept
		{
			return g.Hash();
		}
	};
}
//
//namespace Engine::GUIDHelper
//{
//
//
//	// 新たにGUIDを作成する
//	UUID CreateGUID();
//
//	// 文字列との変換
//	std::string ToString(UUID a_guid);
//	UUID FromString(std::string a_stringGuid);
//
//	constexpr UUID DEFAULT_GUID = {};
//}