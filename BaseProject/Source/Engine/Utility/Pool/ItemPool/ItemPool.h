#pragma once

namespace Engine::Pool
{
	template<typename T>
	class ItemPool
	{
	public:
		ItemPool() = default;
		~ItemPool() = default;

		/// <summary>
		/// 解放処理
		/// </summary>
		void Release();

		/// <summary>
		/// 領域確保
		/// </summary>
		/// <param name="a_capacity">確保したい最大サイズ</param>
		void Reserve(size_t a_capacity);

		/// <summary>
		/// プールに追加
		/// </summary>
		/// <param name="a_resource">登録する実態</param>
		/// <returns>登録して生成したハンドルを返す</returns>
		Handle<T> Add(T&& a_resource);

		/// <summary>
		/// プールから削除 : 参照数を無視して削除するため、複数をまたいで管理するようなものには
		/// FreeRefのほうを使用すること
		/// </summary>
		/// <param name="a_handle">削除したいデータのハンドル</param>
		void Remove(const Handle<T>& a_handle);

		/// <summary>
		/// ポインタ参照
		/// </summary>
		/// <param name="a_handle">参照したいハンドル</param>
		/// <returns>ポインタで返る</returns>
		T* Ref(const Handle<T>& a_handle);

		/// <summary>
		/// 読み取り専用参照
		/// </summary>
		/// <param name="a_handle">ハンドル</param>
		/// <returns>const ポインタで変える</returns>
		const T* Get(const Handle<T>& a_handle) const;

		/// <summary>
		/// 読み取り専用すべてのデータを配列ごと参照
		/// </summary>
		const std::vector<std::optional<T>>& GetAll() const;
		/// <summary>
		/// すべてのデータを配列ごと参照
		/// </summary>
		std::vector<std::optional<T>>& RefAll();

		/// <summary>
		/// インデックスアクセス : チェックができないので危険
		/// </summary>
		/// <param name="a_index">ハンドルインデックス</param>
		/// <returns>ポインタ</returns>
		const T* Access(uint16_t a_index) const;

		/// <summary>
		/// インデックスから世代を取得
		/// </summary>
		/// <param name="a_index"></param>
		/// <returns></returns>
		uint16_t GetGeneration(uint16_t a_index) const;

		/// <summary>
		/// プール内に存在するかのチェック : 実体の内部は考慮しない
		/// </summary>
		/// <param name="a_handle">確認したいハンドル</param>
		/// <returns>存在するのならば true </returns>
		bool IsValid(const Handle<T>& a_handle) const;

	private:

		// データ
		std::vector<std::optional<T>> m_data;		// 実体データ
		std::vector<uint16_t> m_generations;		// 領域の世代
		std::vector<uint16_t> m_freeIndices;		// 使っていない領域
	};


	template<typename T>
	inline void ItemPool<T>::Release()
	{
		m_data.clear();
		m_generations.clear();
		m_freeIndices.clear();
	}

	template<typename T>
	inline void ItemPool<T>::Reserve(size_t a_capacity)
	{
		// 配列のメモリサイズ確保
		m_data.reserve(a_capacity);
		m_generations.reserve(a_capacity);
	}
	template<typename T>
	inline Handle<T> ItemPool<T>::Add(T&& a_resource)
	{
		uint16_t _index = 0;
		// 確保領域にまだ空きがあるのなら
		if (!m_freeIndices.empty())
		{
			// 空き領域を再利用
			_index = m_freeIndices.back();
			m_freeIndices.pop_back();
			m_data[_index] = std::move(a_resource);		// 実体の移動
			m_generations[_index]++;					// 世代を進める
		}
		else
		{
			// インデックスが16bitの上限を超えていないかチェック
			assert(m_data.size() < 0xFFFF && "ItemPoolの最大値が uint16_t のサイズを超えています");

			// 新規追加
			_index = static_cast<uint16_t>(m_data.size());
			m_data.emplace_back(std::move(a_resource));
			m_generations.push_back(1);
		}

		// ハンドルを返す
		Handle<T> _res(_index, m_generations[_index]);
		return _res;
	}
	template<typename T>
	inline void ItemPool<T>::Remove(const Handle<T>& a_handle)
	{
		// 存在チェック
		if (IsValid(a_handle))
		{
			// デストラクタを呼ぶ
			m_data[a_handle.GetIndex()].reset();

			m_freeIndices.push_back(a_handle.GetIndex());	// インデックスキューに返却
			m_generations[a_handle.GetIndex()]++;			// 無効化のため世代を進める
		}
	}
	template<typename T>
	inline T* ItemPool<T>::Ref(const Handle<T>& a_handle)
	{
		// 存在チェック
		if (IsValid(a_handle))
		{
			return &m_data[a_handle.GetIndex()].value();
		}

		// なければnullptr
		return nullptr;
	}
	template<typename T>
	inline const T* ItemPool<T>::Get(const Handle<T>& a_handle) const
	{
		// 存在チェック
		if (IsValid(a_handle))
		{
			return &m_data[a_handle.GetIndex()].value();
		}

		// なければnullptr
		return nullptr;
	}
	template<typename T>
	inline const std::vector<std::optional<T>>& ItemPool<T>::GetAll() const
	{
		return m_data;
	}
	template<typename T>
	inline std::vector<std::optional<T>>& ItemPool<T>::RefAll()
	{
		return m_data;
	}
	template<typename T>
	inline const T* ItemPool<T>::Access(uint16_t a_index) const
	{
		return &m_data[a_index].value();
	}
	template<typename T>
	inline uint16_t ItemPool<T>::GetGeneration(uint16_t a_index) const
	{
		return (a_index < m_generations.size()) ? m_generations[a_index] : 0;
	}
	template<typename T>
	inline bool ItemPool<T>::IsValid(const Handle<T>& a_handle) const
	{
		// インデックスが配列サイズ以上かどうか
		if (a_handle.GetIndex() >= m_data.size())
		{
			return false;
		}
		// ジェネレーションが一致するかどうか
		if (m_generations[a_handle.GetIndex()] != a_handle.GetGeneration())
		{
			return false;
		}

		// データの存在チェック
		if (!m_data[a_handle.GetIndex()].has_value())
		{
			return false;
		}

		// 存在する
		return true;
	}
}