#pragma once
namespace Engine
{
	/// <summary>
	/// デフォルトは uint32_t型 のID StorongTypeクラス
	/// </summary>
	/// <typeparam name="Tag">型</typeparam>
	/// <typeparam name="T">内部数値</typeparam>
	template<class Tag, class T = uint32_t>
	struct ID
	{
		// 実数値 : IDとして判別する用
		T value = std::numeric_limits<T>::max();

		// 空生成用コンストラクタ
		constexpr ID() = default;

		// 初期値を入れてのコンストラクタ
		constexpr explicit ID(T a_value)
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
		constexpr auto operator<=>(const ID&) const = default;
	};
}

namespace std
{
	// マップのキーに使う用のハッシュ
	template<class Tag,class T>
	struct hash<Engine::ID<Tag, T>>
	{
		size_t operator()(const Engine::ID<Tag, T>& a_id) const noexcept
		{
			return std::hash<T>{}(a_id.value);
		}
	};
}