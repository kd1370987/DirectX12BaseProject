#pragma once
namespace Engine
{
	/// <summary>
	/// ハンドル : 型安全でインデックスを管理するための構造体
	/// </summary>
	/// <typeparam name="T">任意の型</typeparam>
	template<typename T>
	struct Handle
	{
		uint32_t id = std::numeric_limits<uint32_t>::max();

		/// <summary>
		/// 空生成
		/// </summary>
		Handle() = default;
		/// <summary>
		/// 初期化生成
		/// </summary>
		/// <param name="a_index">インデックス</param>
		/// <param name="a_generation">世代</param>
		Handle(uint16_t a_index, uint16_t a_generation)
		{
			// 前半が世代、後半がインデックスで作成
			id = (static_cast<uint32_t>(a_generation) << 16) | a_index;
		}

		/// <summary>
		/// スタートインデックスの都合上変えたい場合に使う
		/// 非推奨なのでできるだけ使わない設計で組み立てること
		/// 使うことがなくなれば削除予定
		/// </summary>
		/// <param name="a_index">新規インデックス</param>
		void SetIndex(uint16_t a_index)
		{
			auto _gen = GetGeneration();
			id = (static_cast<uint32_t>(_gen) << 16) | a_index;
		}

		/// <summary>
		/// Index取得
		/// </summary>
		/// <returns>uint16_tのインデックス</returns>
		uint16_t GetIndex() const { return static_cast<uint16_t>(id & 0xFFFF); }

		/// <summary>
		/// Generation取得
		/// </summary>
		/// <returns>uint16_tの世代</returns>
		uint16_t GetGeneration() const { return static_cast<uint16_t>(id >> 16); }

		/// <summary>
		/// 有効判定
		/// </summary>
		bool IsValid() const { return id != std::numeric_limits<uint32_t>::max(); }

		// 比較演算子
		bool operator == (const Handle & a_other) const { return id == a_other.id; }
	};

	/// <summary>
	/// レンジハンドル : 連続した領域を確保して型安全にアクセスするための構造体
	/// </summary>
	/// <typeparam name="T">任意の型</typeparam>
	template<typename T>
	struct RangeHandle
	{
		uint32_t startIndex = std::numeric_limits<uint32_t>::max();
		uint32_t count = 0;
		uint32_t generation = 0;

		/// <summary>
		/// 有効判定
		/// </summary>
		bool IsValid() const { return startIndex != std::numeric_limits<uint32_t>::max() && count > 0; }
	};
}
// ========================================================================================================
// std::map系のキーにするためハッシュ関数を登録
// ========================================================================================================
namespace std
{
	// ハンドルのハッシュ登録
	template<typename T>
	struct hash<Engine::Handle<T>>
	{
		std::size_t operator()(const Engine::Handle<T>& a_handle) const
		{
			// 内部のidをハッシュ関数に投げる
			return std::hash<uint32_t>()(a_handle.id);
		}
	};
}
