#pragma once

namespace Engine::Resource
{
	/// <summary>
	/// アセットローダーの基底クラス
	/// </summary>
	template<typename T, typename DerivedLoader>
	class BaseLoader
	{
	public:

		/// <summary>
		/// 読み込まれているかどうか
		/// </summary>
		/// <returns>ハンドルが存在すれば true</returns>
		static bool Has(const Engine::GUID& a_guid);

		/// <summary>
		/// GUIDとハンドルのキャッシュすべてを参照
		/// </summary>
		static const std::unordered_map<Engine::GUID, Handle<T>>& GetAllGUIDCache() { return m_cache; }

	protected:

		static std::unordered_map<Engine::GUID, Handle<T>> m_cache;			// GUIDとハンドルを紐づけるキャッシュ
	};

	// 静的メンバの実体化
	template <typename T, typename DerivedLoader>
	std::unordered_map<Engine::GUID, Handle<T>> BaseLoader<T, DerivedLoader>::m_cache;

	template<typename T, typename DerivedLoader>
	inline bool BaseLoader<T, DerivedLoader>::Has(const Engine::GUID& a_guid)
	{
		return m_cache.find(a_guid) != m_cache.end();
	}
}