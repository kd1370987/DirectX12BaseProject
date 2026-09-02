#pragma once
namespace Engine
{
	/// <summary>
	/// デフォルトは uint32_t型 のIndex StorongTypeクラス
	/// </summary>
	/// <typeparam name="Tag">型</typeparam>
	/// <typeparam name="T">内部数値</typeparam>
	template<class Tag, class T = uint32_t>
	struct Index
	{
		// 実数値 : Indexとして判別する用
		T value = std::numeric_limits<T>::max();

		// 空生成用コンストラクタ
		constexpr Index() = default;

		// 初期値を入れてのコンストラクタ
		constexpr explicit Index(T a_value)
			: value(a_value)
		{}

		/// <summary>
		/// 有効チェック
		/// </summary>
		/// <returns>正常値なら true</returns>
		[[nodiscard]]
		constexpr bool IsValid() const
		{
			return value != std::numeric_limits<T>::max();
		}

		// 同型どうしのオペレーター
		constexpr auto operator<=>(const Index&) const = default;
	};
}

namespace std
{
	// マップのキーに使う用のハッシュ
	template<class Tag, class T>
	struct hash<Engine::Index<Tag, T>>
	{
		size_t operator()(const Engine::Index<Tag, T>& a_idx) const noexcept
		{
			return std::hash<T>{}(a_idx.value);
		}
	};
}