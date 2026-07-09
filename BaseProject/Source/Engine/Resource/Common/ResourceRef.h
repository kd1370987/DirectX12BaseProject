#pragma once
namespace Engine
{
	template<typename T>
	struct ResourceRef
	{
	public:
		// 実装はリソースマネージャーに書かれている
		ResourceRef() = default;
		ResourceRef(Handle<T> a_h);
		~ResourceRef();
		
		// コピー
		ResourceRef(const ResourceRef& a_other);			// コピーコンストラクタ
		ResourceRef& operator=(const ResourceRef& a_other); // コピー代入

		// ムーブ
		ResourceRef(ResourceRef&& a_other) noexcept;			// ムーブコンストラクタ
		ResourceRef& operator=(ResourceRef&& a_other) noexcept; // ムーブ代入演算子

		Handle<T> GetRaw() const { return m_handle; }
		operator Handle<T>() const { return m_handle; }					// Handle<T> への暗黙の型変換演算子
		explicit operator bool() const { return m_handle.IsValid(); }	// if(myModel) みたいに書けるようになる

		// =======================================================
		// Handleの関数を透過的に使えるようにするラッパー
		// =======================================================
		uint16_t GetIndex() const { return m_handle.GetIndex(); }
		uint16_t GetGeneration() const { return m_handle.GetGeneration(); }
		bool IsValid() const { return m_handle.IsValid(); }

		// Handle同士、または ResourceRef同士の比較もできるようにする
		bool operator==(const ResourceRef& a_other) const { return m_handle == a_other.m_handle; }
		bool operator==(const Handle<T>& a_other) const { return m_handle == a_other; }

	private:
		Handle<T> m_handle;
	};
}